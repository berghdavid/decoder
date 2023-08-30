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
	char*	str;
	Param*	next;
};

void free_params(Data* data);

Data* init_data(int max_buf);

/**
   "original": {
      "protocol": "Traccar",
      "host": "onprem.staging.v3.traxmate.io",
      "topic": "device/866344056940484",
      "data": {
        "event": {
          "id": 36073,
          "attributes": {},
          "deviceId": 7,
          "type": "deviceOffline",
          "eventTime": "2023-07-17T11:08:27.320+00:00",
          "positionId": 0,
          "geofenceId": 0,
          "maintenanceId": 0
        },
        "device": {
          "id": 7,
          "attributes": {},
          "groupId": 0,
          "name": "866344056940484",
          "uniqueId": "866344056940484",
          "status": "offline",
          "lastUpdate": "2023-07-17T11:08:27.319+00:00",
          "positionId": 2658,
          "phone": null,
          "model": null,
          "contact": null,
          "category": null,
          "disabled": "false",
          "expirationTime": null
        }
      }
    },
 * 
 * @return int 
 */
void build_forward_req(Data* data);

void free_data(Data* data);

void print_data(Data* data);

int copy_str(char* dest, int dest_s, char* src);

int parse_package(Data* data, char* pack);

#endif
