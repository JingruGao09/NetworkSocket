#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>
#include <time.h>
#include <limits.h>
using namespace std;

int random_select_neigh(int curr_player_id, int num_players){
  int next_player_id;
  if(curr_player_id == 0){
    next_player_id = (rand()>RAND_MAX/2)? (num_players-1):1;
  }
  else if(curr_player_id == num_players-1){
    next_player_id = (rand()>RAND_MAX/2)? (curr_player_id-1):0;
  }
  else{
    next_player_id = (rand()>RAND_MAX/2)? (curr_player_id-1):(curr_player_id+1);
  }
  //printf("Next player is %d\n",next_player_id);
  return next_player_id;
}

int main(int argc, char *argv[])
{
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  
  if (argc != 3) {
      cout << "Syntax: client <hostname>\n" << endl;
      return 1;
  }
  const char *hostname = argv[1];
  const char *port = argv[2];
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

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
  
  //cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
  
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }


  char my_info[512];
  memset(my_info,0,sizeof(my_info));
  recv(socket_fd,my_info,sizeof(my_info),MSG_WAITALL);//changed flag here
  //cout <<my_info<<endl;

  string my_info_mes(my_info);
  string my_id;
  string num_players;
  string num_hops;

  stringstream stream(my_info_mes);
  std::getline(stream,my_id,',');
  std::getline(stream,num_players,',');
  std::getline(stream,num_hops,',');

  cout<<"Connected as player "<<my_id<<" out of "<<num_players<<" total player"<<endl;
  //---------set up as a server for ,i+1,neighbor----

  int playerstatus;
  int player_fd;
  struct addrinfo player_info;
  struct addrinfo *player_info_list;
  char playername[25];
  memset(playername,0,sizeof(playername));
  
  char playerport[512];
  memset(playerport,0,sizeof(playerport));

  memset(&player_info, 0, sizeof(player_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  gethostname(playername,sizeof(playername));//check here
  //printf("gethostname: %s\n",playername);
  
  
  for(int i = 50000;i < 60000; i++){
    sprintf(playerport,"%d",i);
    playerstatus = getaddrinfo(playername, playerport, &player_info, &player_info_list);
    if (playerstatus != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << playername << "," << playerport << ")" << endl;
      return -1;
    }
    player_fd = socket(player_info_list->ai_family, 
		       player_info_list->ai_socktype, 
		       player_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << playername << "," << playerport << ")" << endl;
      return -1;
    }
    
    int yes = 1;
    playerstatus = setsockopt(player_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    playerstatus = bind(player_fd, player_info_list->ai_addr, player_info_list->ai_addrlen);
    if (playerstatus == -1) {
        continue;
    }
    else{
      playerstatus = listen(player_fd, 2);
      if (playerstatus == -1) {
	cerr << "Error: cannot listen on socket" << endl; 
	cerr << "  (" << playername << "," << playerport << ")" << endl;
	return -1;
      }
      string as_server(playername);
      as_server = as_server + "," + std::to_string(i);
      char as_server_mes[512];
      memset(as_server_mes,0,sizeof(as_server_mes));
      strcpy(as_server_mes,as_server.c_str());
      send(socket_fd,as_server_mes,sizeof(as_server_mes),0);
      break;
    }
  }
  //cout <<"---------------------------------"<<endl;
  //cout << "Server set up done!Waiting for connection on port " << playerport<<"from neighbour" << endl;

  //-----------------recevie next player info to connect-------
  char my_next_info[512];
  memset(my_next_info,0,sizeof(my_next_info));
  recv(socket_fd,my_next_info,sizeof(my_next_info),MSG_WAITALL);//changed flag here

  string my_next(my_next_info);
  //cout << my_next<<endl;

  string next_hostname;
  string next_portname;

  stringstream ss(my_next);
  std::getline(ss,next_hostname,',');
  std::getline(ss,next_portname,',');
  //cout << next_hostname<<endl;
  //cout <<next_portname<<endl;
  
  send(socket_fd,"get",4,0);

  int accept_fd;
  int conn_fd;
  
  char ack[4];
  memset(ack,0,sizeof(ack));
  for(int i= 0;i<2;i++){
    recv(socket_fd,ack,sizeof(ack),MSG_WAITALL);//changed flag here
    
    string ack_mes(ack);
    //cout<<ack_mes<<endl;
    if(ack_mes == "ack"){
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      accept_fd = accept(player_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
      if (accept_fd == -1) {
	cerr << "Error: cannot accept connection on socket" << endl;
	return -1;
      }
      //cout << "accepted my i+1"<<endl;
    }
    if(ack_mes == "con"){
      int con_status;
      struct addrinfo next_player_info;
      struct addrinfo *next_player_info_list;
      memset(&next_player_info, 0, sizeof(next_player_info));
      host_info.ai_family   = AF_UNSPEC;
      host_info.ai_socktype = SOCK_STREAM;
      
      con_status = getaddrinfo(next_hostname.c_str(), next_portname.c_str(), &next_player_info, &next_player_info_list);
      if (con_status != 0) {
	cerr << "Error: cannot get address info for host" << endl;
	return -1;
      }
      
      conn_fd = socket(next_player_info_list->ai_family, 
		       next_player_info_list->ai_socktype, 
		       next_player_info_list->ai_protocol);
      if (conn_fd == -1) {
	cerr << "Error: cannot create socket" << endl;
	return -1;
      } 
      
      //cout << "Connecting to " << next_hostname << " on port " << next_portname << "..." << endl;
      
      con_status = connect(conn_fd, next_player_info_list->ai_addr, next_player_info_list->ai_addrlen);
      if (con_status == -1) {
	cerr << "Error: cannot connect to socket" << endl;
	return -1;
      } 
    }
  }

  //cout<<"----done connect with neigh----\n";

  send(conn_fd,"hi! my prev",12,0);
  send(accept_fd,"hi! next",9,0);
  char fir[12];
  char sec[9];

  recv(accept_fd,fir,sizeof(fir),0);
  recv(conn_fd,sec,sizeof(sec),0);

  //cout<<fir<<endl;
  //cout<<sec<<endl;

  //cout<<"----test ring-----\n";

  //cout<<"----RING DONE-----\n";

  send(socket_fd,"Start Game!",13,0);

  srand((unsigned int)time(NULL)+atoi(my_id.c_str()));

  string tracking_mes;
  //char tracking[512];
  char tracking[4096];
  memset(tracking,0,sizeof(tracking));
    
  while(1){
    int terminate = 0;
    fd_set read_fds;
    
    FD_ZERO(&read_fds);
    FD_SET(socket_fd,&read_fds);
    FD_SET(conn_fd,&read_fds);
    FD_SET(accept_fd,&read_fds);

    int maxfd = socket_fd;

    int fd_list[3] = {socket_fd,conn_fd,accept_fd};
    for(int i = 0;i < 3;i++){
      if(maxfd < fd_list[i]){
	maxfd = fd_list[i];
      }
    }

    int ready = select(maxfd+1, &read_fds, NULL,NULL,NULL);
    if(ready == -1){
      cerr<<"Failure on select----player\n";
      return -1;
    }
    if(ready > 0){
      for(int i = 0;i < 3;i++){
	if(FD_ISSET(fd_list[i],&read_fds)){
	  memset(tracking,0,sizeof(tracking));
	  //int s = 0;
	  //while(s<4096){
	  //s = s +
	  int s = recv(fd_list[i],tracking,sizeof(tracking),MSG_WAITALL);//changed flag here
	  //cout<<s<<endl;
	    //if(s==0){
	    //break;
	    //}
	  //}
	  tracking_mes = tracking;
	  //cout << tracking_mes<<endl;
	  if(tracking_mes == "end"){
	    break;
	  }
	  stringstream split(tracking_mes);
	  string rest_hops;
	  string pass_players;
	  std::getline(split,rest_hops,',');
	  std::size_t pos = tracking_mes.find(',');
	  //cout<<pos<<endl;
          //cout<<"TRACKING_MES: "<<tracking_mes<<endl;
	  pass_players = tracking_mes.substr(pos+1);
	  if(pass_players != ""&&pass_players[0]==','){
            pass_players = pass_players.substr(1);
            //cout<<"Not the first time: "<<pass_players<<endl;
          }
	  //cout<<"===Players: "<<pass_players<<endl;
	  //pass_players = tracking_mes.substr(pos + 1);
	  if(atoi(rest_hops.c_str())==0){
	    tracking_mes = tracking_mes + ","+my_id;
	    //cout <<"--"<< tracking_mes<<endl;
	    strcpy(tracking,tracking_mes.c_str());
	    terminate = 1;
	    break;
	  }
	  if(atoi(rest_hops.c_str())>0){
	    tracking_mes.pop_back();
	    int rest_hops_num = atoi(rest_hops.c_str()) - 1;
	    tracking_mes = std::to_string(rest_hops_num)+","+pass_players;
	    int my_curr_id = atoi(my_id.c_str());
	    int next_player_id = random_select_neigh(my_curr_id,atoi(num_players.c_str()));
	    //printf("Sending potato to %d\n",next_player_id);
	    cout<<"Sending potato to "<<next_player_id<<endl;
	    tracking_mes = tracking_mes+","+my_id;
	    strcpy(tracking,tracking_mes.c_str());
	    int diff = next_player_id - my_curr_id;
	    if(diff==1||diff==1-atoi(num_players.c_str())){
	      send(conn_fd,tracking,sizeof(tracking),0);
	    }
	    else if(diff == -1||diff==atoi(num_players.c_str()) - 1){
	      send(accept_fd,tracking,sizeof(tracking),0);
	    }
	    
	    if(rest_hops_num==0){
	      break;
            }
	  }
	  break;
	}
      }
    }
    if(terminate==1){
      if(tracking[0]>='0'&&tracking[0]<='9'){
      cout<<"I'm it!"<<endl;
      //cout<<tracking<<endl;
      send(socket_fd,tracking,sizeof(tracking),0);
      //send(socket_fd,"End",4,0);
      break;
      }
    }
    if(tracking_mes=="end"){
      break;
    }
    //break;
  }
 
  freeaddrinfo(host_info_list);
  freeaddrinfo(player_info_list);
  close(socket_fd);
  close(accept_fd);
  close(conn_fd);
  return 0;
}
