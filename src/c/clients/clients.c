#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "clients.h"
#include "../utils/queue.h"

void free_client(Client* c)
{
	Worker*	w;
	int	i;

	for (i = 0; i < c->work_s; i++) {
		w = c->worker[i];
		queue_destroy(w->q);
		free(w->buf_rc);
		free(w->buf_sd);
		free(w);
	}
	free(c->worker);
	free(c->thr);
	free(c);
}

void print_received(Worker* w)
{
	char*		str;
	size_t		len;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[32];

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%Y-%m-%d %H:%M:%S", info_t);
	printf("%s [ Client %d < Server ]: ", t_str, w->id);
	
	str = w->buf_rc;
	len = strlen(str);
	if (len >= 2 && str[len - 2] == '\r' && str[len - 1] == '\n') {
		/* Print the string without the last two characters */
		printf("%.*s", (int) (len - 2), str);
	} else {
		printf("%s", str);
	}
	printf("\n\n");
}

void print_sent(Worker* w)
{
	char*		str;
	size_t		len;
	time_t		curr_t;
	struct tm*	info_t;
	char		t_str[32];

	time(&curr_t);
	info_t = localtime(&curr_t);
	strftime(t_str, sizeof(t_str), "%Y-%m-%d %H:%M:%S", info_t);
	printf("%s [ Client %d > Server ]: ", t_str, w->id);
	
	str = w->buf_sd;
	len = strlen(str);
	if (len >= 2 && str[len - 2] == '\r' && str[len - 1] == '\n') {
		/* Print the string without the last two characters */
		printf("%.*s", (int) (len - 2), str);
	} else {
		printf("%s", str);
	}
	printf("\n\n");
}

void* work(void* arg)
{
	Worker*	w;
	char*	str;

	w = (Worker*) arg;

	/* TODO: Thread unsafe */
	while ((str = deq(w->q)) != NULL) {
		w->socket = socket(PF_INET, SOCK_STREAM, 0);
		strcpy(w->buf_sd, str);
		free(str);

		if (connect(w->socket, (Sockaddr*) &w->client->sockad, 
			sizeof(w->client->sockad)) < 0) {
			printf("Unsuccessful connection...\n");
			return NULL;
		}

		send(w->socket, w->buf_sd, sizeof(char) * w->client->buf_s, 0);
		print_sent(w);
		recv(w->socket, w->buf_rc, sizeof(char) * w->client->buf_s, 0);
		close(w->socket);
		print_received(w);
	}
	return NULL;
}

void start_workers(Client* c)
{
	int	i;

	for (i = 0; i < c->work_s; i++) {
		pthread_create(&c->thr[i], NULL, work, c->worker[i]);
	}

	/* Wait for all threads, then join them when finished */
	for (i = 0; i < c->work_s; i++) {
		if (pthread_join(c->thr[i], NULL) != 0) {
			printf("Could not join with thread %d\n", i);
		}
	}
}

Worker* init_worker(Client* c, int id)
{
	Worker*	w;

	w = malloc(sizeof(Worker));
	w->client = c;
	w->id = id;
	w->addr = NULL;
	w->addr_s = 0;
	w->socket = 0;
	w->buf_rc = calloc(2048, sizeof(char));
	w->buf_sd = calloc(2048, sizeof(char));
	w->q = queue_create(2048);
	return w;
}

void init_workers(Client* c, int workers)
{
	int	i;
	int	j;
	char*	fifo[12] = {
		"$$159,866344056951341,399D,A03,,230716222659,240|8|2724|20EEF33,4.20,100,003E,1,AE233FC0D2E0:-65|3E286D5FB6E8:-65|28BD890A4A0E:-67|8ED81B5DFC3A:-70|8AD81B5DFC3A:-70*5F",
		"$$95,866104023192332,1,A03,,210414055249,460|0|25FC|104C,4.18,100,000F,0,A,2,9,22.643175,114.018150*75",
		"$$136,866104023192332,1,A03,,210414055249,460|0|25FC|104C,4.18,100,000F,1,94D9B377EB53:-60|EC6C9FA4CAD8:-55|CA50E9206252:-61|54E061260A89:-51*3E",
		"$$274,863003046499158,18D0,A01,,211026081639,A,13.934116,100.000463,0,263,16,366959,345180,80000040,02,0,520|0|FA8|1A9B5B9,9DE|141|2D,%  ^YENSABAICHAI$SONGKRAN$MR.^^?;6007643190300472637=150519870412=?+             14            1            0000155  00103                     ?,*69",
		"$$118,863003046473534,258,A01,,201007231735,V,3.067783,101.672858,0,176,96,189890,0,A0,03,0,502|19|5C1|93349F,196|4E0|6C,1,*13",
		"$$116,869270049149999,5,A01,4,190925080127,V,-15.804260,35.061506,0,0,1198,0,0,900000C0,02,0,650|10|12C|B24,18B|4C8|72,1,*01",
		"$$79,868345037864709,382,D05,190220085833,22.643210,114.018176,1,1,1,13152,23FFD339*25",
		"$$105,866104023179743,AB,A00,,161007085534,A,54.738791,25.271918,0,350,151,0,17929,0000,0,,246|1|65|96DB,936|0*0B",
		"$$103,866104023179743,5,A00,,161006192841,A,54.738791,25.271918,0,342,200,0,4265,0000,0,,246|1|65|96DB,9C4|0*75",
		"$$103,866104023179743,4,A00,,161006192810,V,54.738791,25.271918,0,158,122,0,4235,0000,0,,246|1|65|96DB,9C5|0*69",
		"$$135,866104023192332,29,A01,,160606093046,A,22.546430,114.079730,0,186,181,0,415322,0000,02,2,460|0|27B3|EA7,A2F|3B9|3|0,940C7E,31.76|30.98*46",
		"q"
	};
	/*
	char*	meitrack[13] = {
		"$$F160,861412043027965,AAA,22,45.499458,-82.493581,220718171428,V,0,0,0,0,0.0,0,227940,119812,302|220|D8D6|086E1B2B,0000,0000|0000|0000|0191|0573,,,3,,002134,0,0*FA",
		"$$D149,867047043162018,AAA,35,-1.264865,36.800705,211001105240,A,9,20,41.0,323,1,1697,1,0,000|00||,0000,4.33|12.96|1.92|2.72|2.69,0.000000|0|0.000000,*E1",
		"$$F153,867144025101013,AAA,35,25.219431,55.279918,200916155923,V,0,25,0,0,0.0,0,249701532,98374503,424|2|101C|A3AE,0800,0000|0000|0000|02D3|0103,00000011,*A0",
		"$$O160,863835028611502,AAA,35,7.887840,98.375193,200202020238,A,12,4,0,279,0.6,45,32121,442492,520|3|12DF|015273E2,0000,0000|0000|0000|018D|04F0,00000001,,1,0000*F3",
		"$$^182,864507031245110,AAA,109,13.844553,100.644360,171227173141,A,11,19,0,359,0.8,8,15075,934591,520|4|0643|07D20555,8400,0000|0000|0000|018D|04CB,,,108,0000,,6,0,,,,,10|171227173100*7C",
		"$$S214,864507031219974,AAA,109,13.844643,100.644395,171207021520,A,10,28,0,31,0.8,6,390,421327,520|0|0016|000F2DB0,8400,0000|0000|0000|018D|04C6,,,108,0000,,6,0,,,,,11|171207091500|171207091500|78|3500|000000|000003*12",
		"$$V177,863835026871173,AAA,35,34.516428,10.470160,170915154043,A,9,12,68,74,0.9,9,1988259,525882,605|2|008C|0007B5A6,0200,0003|0000|0000|01A6|0571,00000001,,3,0000,06FB2E,360,511*74",
		"$$V177,863835026871173,AAA,35,34.516428,10.470160,170915154043,A,9,12,68,74,0.9,9,1988259,525882,605|2|008C|0007B5A6,0200,0003|0000|0000|01A6|0571,00000001,,3,0000,010A92,360,511*74",
		"$$B136,011691002364761,AAA,29,47.055220,28.893193,170914144240,V,0,7,0,0,0,132,129754946,129793197,259|2|02F8|413F,0000,000D|000C||028C|,*9E",
		"$$F153,863835026880190,AAA,29,25.313160,55.422473,170628150902,V,0,0,0,0,0.0,0,6553,6697,0|0|0000|00000000,0000,0002|0000|0000|018B|0000,,,3,0000,,110,386*22",
		"$$T143,869013024733944,AAA,1,18.459575,-69.947161,170220142912,A,5,15,10,300,1.6,115,3989,187884,370|2|5337|2B2C,0100,0000|0000|0000|0964|0B04,,*C2",
		"$$K157,866771027160687,AAA,3,37.040231,10.042391,160412151656,A,10,11,0,48,0.8,21,1035518,774980,605|2|0010|307B,0400,0000|0000|0000|0A47|03E3,,,1,0000,001206*2C",
		"$$D28,353358017784062,D03,OK*F3"
	};*/

	c->work_s = workers;
	c->worker = malloc(workers * sizeof(Worker*));
	c->thr = malloc(workers * sizeof(pthread_t));
	for (i = 0; i < workers; i++) {
		c->worker[i] = init_worker(c, i);
	}

	for (i = 0; i < 12; i++) {
		j = i % workers;
		enq(c->worker[j]->q, fifo[i]);
	}
}

Client* init_client(int workers)
{
	Client*	c;

	c = malloc(sizeof(Client));
	c->buf_s = 2048;
	c->host = "localhost";
	c->port = 5124;
	c->work_s = 0;
	c->worker = NULL;
	c->thr = NULL;

	memset(&c->sockad, '\0', sizeof(c->sockad));
	c->sockad.sin_family = AF_INET;
	c->sockad.sin_port = htons(c->port);
	c->sockad.sin_addr.s_addr = inet_addr(c->host);

	init_workers(c, workers);
	return c;
}

int main(int argc, char* argv[])
{
	Client*	c;

	c = init_client(1);
	start_workers(c);
	free_client(c);
	return 0;
}
