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
	Param*	cmd_para;	/* Command parameters in linked list	*/
	char*	checksum;	/* Checksum of package, 2 bytes		*/
};

struct Param {
	char*	str;
	Param*	next;
};

Data* init_data();

void free_data(Data* data);

void print_data(Data* data);

int parse_package(Data* data, char* pack);

#endif
