#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

char* PARAMS_A03[] = {
	"alarm-param", "date-time", "MCC|MNC|LAC|CI", "bat-v", "bat-level", "status",
	"loc-type", "info"
};
char* PARAMS_A10[] = {
	"status", "bat-ad"
};
char* PARAMS_GPS[] = {
	"fix-flag", "speed", "salt-num", "lat", "lon"
};
char* PARAMS_WIFI[] = {
	"wifi-ap"
};

Data* init_data(int max_buf)
{
	Data*	data;

	data = malloc(sizeof(Data));
	data->cmd_para = NULL;
	data->pack_len = 0;

	data->id_s = 19;
	data->work_nb_s = 5;
	data->cmd_code_s = 4;
	data->para_str_s = max_buf;
	data->checksum_s = 3;
	data->json_s = max_buf + 1000;

	data->id = calloc(data->id_s, sizeof(char));
	data->work_nb = calloc(data->work_nb_s, sizeof(char));
	data->cmd_code = calloc(data->cmd_code_s, sizeof(char));
	data->para_str = calloc(data->para_str_s, sizeof(char));
	data->checksum = calloc(data->checksum_s, sizeof(char));
	data->json = calloc(data->json_s, sizeof(char));
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
		free(tmp->key);
		free(tmp->val);
		free(tmp);
	}	
}

void free_data(Data* data)
{
	free(data->id);
	free(data->work_nb);
	free(data->cmd_code);
	free(data->para_str);
	free_params(data);
	free(data->checksum);
	free(data->json);
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
		printf("%s: %s", p->key, p->val);
		p = p->next;
	}
	printf(" ]\n");
	printf("Checksum:\t %s\n", data->checksum);
	printf("\n");
}

Param* parse_gps(Param* fix_flag)
{
	Param*	p;
	int	i;
	
	p = fix_flag->next;
	for (i = 0; i < 5; i++) {
		if (p == NULL) {
			break;
		}
		strcpy(p->key, PARAMS_GPS[i]);
		p = p->next;
	}
	return p;
}

Param* parse_wifi(Param* fix_flag)
{
	Param*	p;

	p = fix_flag->next;
	if (p != NULL) {
		strcpy(p->key, PARAMS_WIFI[0]);
	}
	return p->next;
}

void parse_cmd_A03(Data* data)
{
	Param*	p;
	int	i;

	p = data->cmd_para;
	i = 0;
	while (p != NULL) {
		if (i < 6) {
			strcpy(p->key, PARAMS_A03[i]);
		} else if (i == 6) {
			/* loc-type */
			strcpy(p->key, PARAMS_A03[i]);
			if (*p->val == '0') {
				p = parse_gps(p);
			} else if (*p->val == '1') {
				p = parse_wifi(p);
			}
			continue;
		} else if (i == 7) {
			strcpy(p->key, PARAMS_A03[i]);
			break;
		}
		i++;
		p = p->next;
	}
}

void parse_cmd_A10(Data* data)
{
	Param*	p;
	int	i;

	p = data->cmd_para;
	for (i = 0; i < 2; i++) {
		if (p == NULL) {
			break;
		}
		strcpy(p->key, PARAMS_A10[i]);
		p = p->next;
	}
}

void parse_cmd(Data* data)
{
	if (strcmp(data->cmd_code, "A03") == 0) {
		parse_cmd_A03(data);
	} else if (strcmp(data->cmd_code, "A10") == 0) {
		parse_cmd_A10(data);
	}
}

void parse_params(Data* data)
{
	const char	com_s[2] = ",";
	Param*		p;
	char*		buf;
	int		first;

	first = 1;
	buf = data->para_str;
	p = malloc(sizeof(Param));
	p->key = NULL;
	p->val = NULL;
	while (*buf == ',') {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		} else {
			data->cmd_para = p;
			first = 0;
		}
		p->next = NULL;
		p->key = calloc(512, sizeof(char));
		p->val = calloc(512, sizeof(char));
		strcpy(p->key, "\0");
		strcpy(p->val, "\0");
		buf++;
	}
	buf = strtok(buf, com_s);
	while (buf != NULL) {
		if (!first) {
			p->next = malloc(sizeof(Param));
			p = p->next;
		} else {
			data->cmd_para = p;
			first = 0;
		}
		p->key = calloc(512, sizeof(char));
		p->val = calloc(512, sizeof(char));
		if (copy_str(p->val, 512, buf)) {
			fprintf(stderr, "Error - parameter: '%s' is larger than 512 bytes.\n", buf);
		}
		buf = strtok(NULL, com_s);
	}
	p->next = NULL;
}

void strip(char* str)
{
	char	c;

	c = str[strlen(str) - 1];
	while (c == '\n' || c == '\r') {
		str[strlen(str) - 1] = '\0';
		c = str[strlen(str) - 1];
	}
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
		fprintf(stderr,	"Error - no initial $$ in package '%s'\n", pack);
		return 1;
	}

	i = 0;
	buf = strtok(pack, com_s);
	while (buf != NULL) {
		switch(i) {
			case 0:
				/* Skip $$ */
				buf += 2;
				if (sscanf (buf, "%i", &data->pack_len) != 1) {
					fprintf(stderr, "Error - pack_len '%s' is not an integer.\n", buf);
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
				/* Remove last 2 chars \r\n */
				c = buf[strlen(buf) - 1];
				if (c == '\n' || c == '\r') {
					buf[strlen(buf) - 1] = '\0';
				}
				c = buf[strlen(buf) - 1];
				if (c == '\n' || c == '\r') {
					buf[strlen(buf) - 1] = '\0';
				}
				if (copy_str(data->checksum, data->checksum_s, buf)) {
					fprintf(stderr, "Error - checksum string: '%s' is too large.\n", buf);
					return 1;
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
	parse_cmd(data);
	return 0;
}
