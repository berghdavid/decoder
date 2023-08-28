#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

Data* init_data()
{
	Data*	data;

	data = malloc(sizeof(Data));
	data->pack_len = 0;
	data->id = calloc(19, sizeof(char));
	data->work_nb = calloc(5, sizeof(char));
	data->cmd_code = calloc(4, sizeof(char));
	data->para_str = calloc(1025, sizeof(char));
	data->cmd_para = malloc(sizeof(Param));
	data->cmd_para->next = NULL;
	data->cmd_para->str = calloc(128, sizeof(char));
	data->checksum = calloc(3, sizeof(char));
	return data;
}

void free_param(Param* p)
{
	Param*	i;
	Param*	tmp;

	i = p;
	while (i != NULL) {
		tmp = i;
		i = i->next;
		free(tmp->str);
		free(tmp);
	}	
}

void free_data(Data* data)
{
	free(data->id);
	free(data->work_nb);
	free(data->cmd_code);
	free(data->para_str);
	free_param(data->cmd_para);
	free(data->checksum);
	free(data);
}

void print_data(Data* data)
{
	Param*	p;

	printf(" --- Package ---\n");
	printf("Package length:\t %d\n", data->pack_len);
	printf("Id:\t\t %s\n", data->id);
	printf("Work number:\t %s\n", data->work_nb);
	printf("Command code:\t %s\n", data->cmd_code);
	printf("Command p_string:\t %s\n", data->para_str);
	printf("Command params:\t [ ");
	p = data->cmd_para;
	while (p != NULL) {
		if (p != data->cmd_para) {
			printf(", ");
		}
		printf("%s", p->str);
		p = p->next;
	}
	printf(" ]\n");
	printf("Checksum:\t %s\n", data->checksum);
}

void parse_params(Data* data)
{
	const char	com_s[2] = ",";
	Param*		p;
	char*		buf;
	int		first;

	first = 1;
	p = data->cmd_para;
	buf = data->para_str;
	while (*buf == ',') {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		}
		first = 0;
		p->str = calloc(128, sizeof(char));
		strcpy(p->str, "\0");
		buf++;
	}
	buf = strtok(buf, com_s);
	while (buf != NULL) {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		}
		first = 0;
		p->str = calloc(128, sizeof(char));
		strcpy(p->str, buf);
		buf = strtok(NULL, com_s);
	}
	p->next = NULL;
}

/*
 * Example request:
 * 	$$159,866344056940484,2E69,A03,,230824200543,240|8|2724|20EEF33,4.21,100,003F,1,
 * 	84D81B5DFC3A:-66|8ED81B5DFC3A:-66|8AD81B5DFC3A:-67|AC233FC0D496:-68|3C286D5FBD72:-68*55
*/
int parse_package(Data* data, char* pack)
{
	/* TODO: read strcpy return msg */
	const char	com_s[2] = ",";
	const char	ast_s[2] = "*";
	int		i;
	char		c;
	char*		buf;

	if (strlen(pack) < 15 || pack[0] != '$' || pack[1] != '$') {
		fprintf(stderr,	"Error when parsing '%s'.\n", pack);
		return 1;
	}

	i = 0;
	buf = strtok(pack, com_s);
	while (buf != NULL) {
		switch(i) {
			case 0:
				buf += 2; /* Remove $$ */
				if (sscanf (buf, "%i", &data->pack_len) != 1) {
					fprintf(stderr, "Error - Packet length '%s' not an integer.\n", buf);
					return 1;
				}
				break;
			case 1:
				strcpy(data->id, buf);
				break;
			case 2:
				strcpy(data->work_nb, buf);
				break;
			case 3:
				strcpy(data->cmd_code, buf);
				break;
			case 4:
				strcpy(data->para_str, buf);
				parse_params(data);
				break;
			case 5:
				strcpy(data->checksum, buf);
				/* Remove last 2 chars \r\n */
				c = data->checksum[strlen(data->checksum) - 1];
				if (c == '\n' || c == '\r') {
					data->checksum[strlen(data->checksum) - 1] = '\0';
				}
				c = data->checksum[strlen(data->checksum) - 1];
				if (c == '\n' || c == '\r') {
					data->checksum[strlen(data->checksum) - 1] = '\0';
				}
				return 0;
		}
		if (i < 3) {
			buf = strtok(NULL, com_s);
		} else {
			buf = strtok(NULL, ast_s);
		}
		printf("val: %s\n", buf);
		i++;
	}
	return 0;
}
