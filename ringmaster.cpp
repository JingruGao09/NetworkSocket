#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include "potato.hpp"
using namespace std;

int main(int argc, char *argv[])
{
  //---------check command line input------
  if(argc!= 4){
    cerr<<"Please enter right command line"<<endl;
    return -1;
  }
  const char *port = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);

  cout<<"Potato Ringmaster"<<endl;
  cout<<"Players = "<<num_players<<endl;
  cout<<"Hops = "<<num_hops<<endl;
  
  if(num_players <= 1){
    perror("Players number must be greater than 1!\n");
    return -1;
  }
  if(num_hops<0 || num_hops >512){
    perror("Hops number must between 0 and 512!\n");
    return -1;
  }

  //---------set up server--------------
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  //cout << "accept all players connections " << port << endl;
  
  //-----------accept all players connections-------
  int players_conn_fd[num_players];
  struct player_info player[num_players];
  for(int i = 0;i<num_players;i++){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
   
    players_conn_fd[i] = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (players_conn_fd[i] == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      return -1;
    }
      
    string players_info = std::to_string(i) + "," + std::to_string(num_players)+","+std::to_string(num_hops);
        
    char players_info_mes[512];
    memset(players_info_mes,0,512);
    strcpy(players_info_mes,players_info.c_str());
    send(players_conn_fd[i],players_info_mes,512,0);
    
    char players_server_mes[512];
    memset(players_server_mes,0,sizeof(players_server_mes));
    recv(players_conn_fd[i],players_server_mes,sizeof(players_server_mes),MSG_WAITALL);//changed flag here

    memset(player[i].my_ser_info,0,sizeof(player[i].my_ser_info));
    strcpy(player[i].my_ser_info,players_server_mes);
    player[i].id = i;
    cout<<"Player "<<i<<" is ready to play"<<endl;
    //printf("%s\n",player[i].my_ser_info);
  }

  //xcout<<"---------player connect with master & ready to build ring-----\n";

  char ack[4];
  memset(ack,0,sizeof(ack));
  for(int i = 0;i <num_players;i++){
    if(i != num_players-1){
      send(players_conn_fd[i],player[i+1].my_ser_info,sizeof(player[i+1].my_ser_info),0);
    }
    else{
      send(players_conn_fd[i],player[0].my_ser_info,sizeof(player[0].my_ser_info),0);
    }
    recv(players_conn_fd[i],ack,sizeof(ack),MSG_WAITALL);//changed flag here
    //cout<<ack<<endl;
  }

  //cout<<"----all player recive their i+1 port_num & hostname----\n";

  for(int i = 0;i <num_players;i++){
    if(i != num_players-1){
      send(players_conn_fd[i],"ack",4,0);
      send(players_conn_fd[i+1],"con",4,0);
    }
    else{
      send(players_conn_fd[i],"ack",4,0);
      send(players_conn_fd[0],"con",4,0);
    }
  }

  char sign_start[13];
  memset(sign_start,0,sizeof(sign_start));

  for(int i = 0;i <num_players;i++){
    recv(players_conn_fd[i],sign_start,sizeof(sign_start),MSG_WAITALL);//changed flag here
    
    //cout<<"Player "<<i<<" is ready to play"<<endl;
  }

  //----------num_hops = 0----------
  if(num_hops == 0){
    cout<<"Trace of potato:"<<endl;
    cout<<"\n";
    for(int i = 0;i < num_players;i++){
      send(players_conn_fd[i],"end",4,0);
    }
    for(int i = 0;i < num_players;i++){
      close(players_conn_fd[i]);
    }
    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
  }
  //---------end deal num_hops = 0-----------
  
  //cout<<"-------------------------"<<endl;
  //cout<<"\n";
  //cout<< sign_start<<endl;
  //cout<<"We can chose the first player now!\n";

  srand((unsigned int)time(NULL)+0);
  int random = rand() % num_players;

  cout<<"Ready to start the game, sending potato to player "<<random<<endl;

  string tracking_mes = std::to_string(num_hops-1) + ",";
  //cout<<"----First Tracking is "<<tracking_mes<<endl;

  //char tracking[512];
  char tracking[4096];
  memset(tracking,0,sizeof(tracking));
  strcpy(tracking,tracking_mes.c_str());
  send(players_conn_fd[random],tracking,sizeof(tracking),0);
  //cout<<"Start the game, send potato to "<<random<<" with tracking "<<tracking_mes<<endl;

  char end_game[4096];
  memset(end_game,0,sizeof(end_game));
  strcpy(end_game,"end");
  //cout<<sizeof(end_game)<<endl;
  while(1){
    fd_set read_fds;
    
    FD_ZERO(&read_fds);
    int maxfd = players_conn_fd[0];
    for(int i = 0;i < num_players; i++){
      FD_SET(players_conn_fd[i],&read_fds);
      if(maxfd < players_conn_fd[i]){
	maxfd = players_conn_fd[i];
      }
    }
    int ready = select(maxfd+1, &read_fds, NULL,NULL,NULL);
    if(ready == -1){
      cerr<<"Failure on select----ringmaster\n";
      return -1;
    }
    if(ready > 0){
      for(int i = 0;i < num_players;i++){
	if(FD_ISSET(players_conn_fd[i],&read_fds)){
	  //char end_game[4];
	  //memset(end_game,0,sizeof(end_game));
	  recv(players_conn_fd[i],tracking,sizeof(tracking),MSG_WAITALL);//changed flag here
	  //recv(players_conn_fd[i],end_game,sizeof(end_game),0);
	  //cout<<"Player "<<i<<endl;
	  tracking_mes = tracking;
	  std::size_t pos = tracking_mes.find(',');
	  //tracking_mes = tracking_mes.substr(pos+2);
	  tracking_mes = tracking_mes.substr(pos+1);
	  cout<<"Trace of potato:"<<endl;
	  cout<<tracking_mes<<endl;
	  //cout<<end_game<<endl;
	  break;
	}
      }
      for(int i = 0;i < num_players;i++){
	send(players_conn_fd[i],end_game,sizeof(end_game),0);
	//send(players_conn_fd[i],"end",4,0);
      }
      for(int i = 0;i < num_players;i++){
	close(players_conn_fd[i]);
      }
      break;
    }
  }
  freeaddrinfo(host_info_list);
  close(socket_fd);

  return 0;
}
