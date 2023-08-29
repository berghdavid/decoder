#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

Data* init_data()
{
	Data*	data;

	data = malloc(sizeof(Data));
	data->cmd_para = NULL;
	data->pack_len = 0;

	data->id_s = 19;
	data->work_nb_s = 5;
	data->cmd_code_s = 4;
	data->para_str_s = 2049;
	data->checksum_s = 3;

	data->id = calloc(data->id_s, sizeof(char));
	data->work_nb = calloc(data->work_nb_s, sizeof(char));
	data->cmd_code = calloc(data->cmd_code_s, sizeof(char));
	data->para_str = calloc(data->para_str_s, sizeof(char));
	data->checksum = calloc(data->checksum_s, sizeof(char));
	return data;
}

void free_params(Data* data)
{
	Param*	i;
	Param*	tmp;

	i = data->cmd_para;
	data->cmd_para = NULL;
	while (i != NULL) {
		tmp = i;
		i = i->next;
		free(tmp->str);
		free(tmp);
	}	
}

void reset_data(Data* data)
{
	free_params(data);
}

void free_data(Data* data)
{
	free(data->id);
	free(data->work_nb);
	free(data->cmd_code);
	free(data->para_str);
	free_params(data);
	free(data->checksum);
	free(data);
}

void print_data(Data* data)
{
	Param*	p;

	printf("\n --- Package ---\n");
	printf("Package length:\t %d\n", data->pack_len);
	printf("Id:\t\t %s\n", data->id);
	printf("Work number:\t %s\n", data->work_nb);
	printf("Cmd code:\t %s\n", data->cmd_code);
	printf("Cmd p_string:\t %s\n", data->para_str);
	printf("Cmd params:\t [ ");
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
	printf("\n");
}

void parse_params(Data* data)
{
	const char	com_s[2] = ",";
	Param*		p;
	char*		buf;
	int		first;

	first = 1;
	buf = data->para_str;
	while (*buf == ',') {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		} else {
			data->cmd_para = malloc(sizeof(Param));
			p = data->cmd_para;
			first = 0;
		}
		p->next = NULL;
		p->str = calloc(128, sizeof(char));
		strcpy(p->str, "\0");
		buf++;
	}
	buf = strtok(buf, com_s);
	while (buf != NULL) {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		} else {
			data->cmd_para = malloc(sizeof(Param));
			p = data->cmd_para;
			first = 0;
		}
		p->str = calloc(128, sizeof(char));
		strcpy(p->str, buf);
		buf = strtok(NULL, com_s);
	}
	p->next = NULL;
}
int copy_str(char* dest, int dest_s, char* src)
{
	if (strlen(src) > dest_s) {
		return 1;
	}
	strcpy(dest, src);
	return 0;
}

int parse_package(Data* data, char* pack)
{
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
				/* Remove $$ */
				buf += 2;
				if (sscanf (buf, "%i", &data->pack_len) != 1) {
					fprintf(stderr, "Error - Packet length '%s' not an integer.\n", buf);
					return 1;
				}
				break;
			case 1:
				if (copy_str(data->id, data->id_s, buf)) {
					fprintf(stderr, "Error - id string: '%s' is too large.\n", buf);
					return 1;
				}
				break;
			case 2:
				if (copy_str(data->work_nb, data->work_nb_s, buf)) {
					fprintf(stderr, "Error - work_number string: '%s' is too large.\n", buf);
					return 1;
				}
				break;
			case 3:
				if (copy_str(data->cmd_code, data->cmd_code_s, buf)) {
					fprintf(stderr, "Error - cmd_code string: '%s' is too large.\n", buf);
					return 1;
				}
				break;
			case 4:
				if (copy_str(data->para_str, data->para_str_s, buf)) {
					fprintf(stderr, "Error - Command parameters: '%s' are too large.\n", buf);
					return 1;
				}
				break;
			case 5:
				if (copy_str(data->checksum, data->checksum_s, buf)) {
					fprintf(stderr, "Error - checksum string: '%s' is too large.\n", buf);
					return 1;
				}
				/* Remove last 2 chars \r\n */
				c = data->checksum[strlen(data->checksum) - 1];
				if (c == '\n' || c == '\r') {
					data->checksum[strlen(data->checksum) - 1] = '\0';
				}
				c = data->checksum[strlen(data->checksum) - 1];
				if (c == '\n' || c == '\r') {
					data->checksum[strlen(data->checksum) - 1] = '\0';
				}
				break;
		}
		if (i < 3) {
			buf = strtok(NULL, com_s);
		} else if (i < 5) {
			buf = strtok(NULL, ast_s);
		} else {
			break;
		}
		i++;
	}
	parse_params(data);
	return 0;
}
