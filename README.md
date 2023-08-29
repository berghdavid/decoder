# Server-Client
Multithreaded server/client implementation in C using Unix pthread.
Based on a task for a work application.

## To run
Enter the project the folder in two separate terminals.
- `make run_server` builds and runs the server application.
- `make run_clients` builds and runs clients sending requests to the server.

Test with: 
```bash
echo -n '$$159,866344056940484,2E69,A03,,230824200543,240|8|2724|20EEF33,4.21,100,003F,1,84D81B5DFC3A:-66|8ED81B5DFC3A:-66|8AD81B5DFC3A:-67|AC233FC0D496:-68|3C286D5FBD72:-68*55' | nc localhost 5142
```
