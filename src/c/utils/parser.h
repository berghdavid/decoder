#ifndef PARSER_H
#define PARSER_H

typedef struct Data Data;
typedef struct Param Param;

struct Data {
	int	pack_len;	/* Package length, decimal string	*/
	char*	id;		/* IMEI ID in format AA-BBBBBB-CCCCCC-D	*/
	char*	work_nb;	/* Working number, hexadecimal string	*/
	char*	cmd_code;	/* Command code				*/
	char*	para_str;	/* Command parameters as string object	*/
	char*	checksum;	/* Checksum of package, 2 bytes		*/
	int	id_s;		/* ID size				*/
	int	work_nb_s;	/* Working number size			*/
	int	cmd_code_s;	/* Command code size			*/
	int	para_str_s;	/* Command parameters size		*/
	int	checksum_s;	/* Checksum size			*/
	Param*	cmd_para;	/* Command parameters in linked list	*/
	char*	json;		/* Built JSON to forward		*/
	int	json_s;		/* JSON size in bytes			*/
};

struct Param {
	char*	key;
	char*	val;
	Param*	next;
};

void free_params(Data* data);

Data* init_data(int max_buf);

void free_data(Data* data);

void print_data(Data* data);

void parse_cmd(Data* data);

int copy_str(char* dest, int dest_s, char* src);

int parse_package(Data* data, char* pack);

#endif
