//FEUP 2021/2022 - Licenciatura em Engenharia Electrotécnica e de Computadores - Redes de Computadores
//Duarte Ribeiro Afonso Branco                  up201905327
//Pedro Afonso da Silva Correia de Castro Lopes up201907097

#include "linklayer.h"

#define FLAG 0x7E
#define A 0x03

#define SET_C 0x03

#define UA_C 0x07

#define I_C0 0x00
#define I_C1 0x02

#define RR_C0 0x01
#define RR_C1 0x21

#define REJ_C0 0x05
#define REJ_C1 0x25

#define C_DISC 0x0B

#define ESC1 0x7D
#define ESC2 0x5E
#define ESC3 0x5D

int fd, alarm_stop=0, conta=1, num_tries=0, time_out=0, sequence_number_transmitter=0, sequence_number_reciever=0,
    sequence_number_old_reciever=1, PACKAGE_NUM_SEND=0, PACKAGE_NUM_RECIEVE=1, role=0, statNum_retransmitted=0,
    statNum_recieved=0, statNum_timeOut=0, statNum_RRsend=0, statNum_RRrecieve=0, statNum_REJsend=0, statNum_REJrecieve=0, error_read=0, error_write=0, statRead_0=0;

int get_baud(int baud)
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default: 
        return -1;
    }
}

void atende()                   
{
  //printf("Alarme # %d\n", conta);
	alarm_stop=1;
	conta++;
  statNum_timeOut++;
  return;
}

int SET_state_machine(int state, unsigned char pos)
{
  switch (state)
  {
  case 0:
    if(pos==FLAG) state=1;
    break;

  case 1:
    if(pos==A) state=2;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 2:
    if(pos==SET_C) state=3;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 3:
    if(pos==A^SET_C) state=4;
    else if(pos==FLAG) state=1;
    else state=0;
    break;

  case 4:
    if(pos==FLAG) state=5;
    else state=0;
    break;

  case 5:
    break;
  }
  return state;
}

int UA_state_machine(int state, unsigned char pos)
{
  switch (state)
  {
  case 0:
    if(pos==FLAG) state=1;
    break;

  case 1:
    if(pos==A) state=2;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 2:
    if(pos==UA_C) state=3;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 3:
    if(pos==A^UA_C) state=4;
    else if(pos==FLAG) state=1;
    else state=0;
    break;

  case 4:
    if(pos==FLAG) state=5;
    else state=0;
    break;

  case 5:
    break;
  }
  return state;
}

int llopen(linkLayer connectionParameters) 
{
  int res=0, res_old=0, state=0, attempt=0;
  struct termios oldtio, newtio;
  
  fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
  if(fd<0) return -1;
  
  if(tcgetattr(fd,&oldtio)==-1) // save fd to oldtio 
  { 
      perror("tcgetattr");
      return -1;
  }

  bzero(&newtio, sizeof(newtio)); // delete sizeof(newtio) bytes in pointer &newtio
  newtio.c_cflag = get_baud(connectionParameters.baudRate) | CS8 | CLOCAL | CREAD; 
  //newtio.c_cflag = CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  // set input mode (non-canonical, no echo,...) 
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 1;   // inter-character timer 
  newtio.c_cc[VMIN]     = 0;   // blocking read until x chars received 
  
  tcflush(fd, TCIOFLUSH);  //discards data in fd, flushes both data received but not read and data written but not transmitted

  if (tcsetattr(fd,TCSANOW,&newtio)==-1) // save newtio to fd
  {
      perror("tcsetattr");
      return -1;
  }

  //printf("New termios structure set\n");
  
  num_tries=connectionParameters.numTries;
  time_out=connectionParameters.timeOut;
  role=connectionParameters.role;
  
  if(connectionParameters.role==TRANSMITTER)
  {
      unsigned char SET_send[5], UA_recieve;
      
      (void) signal(SIGALRM, atende);  

      SET_send[0] = FLAG;
      SET_send[1] = A;
      SET_send[2] = SET_C; 
      SET_send[3] = A^SET_C;
      SET_send[4] = FLAG;

      while(attempt<connectionParameters.numTries)
      {
          res = write(fd,SET_send,5);   
          if(res<=0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
              
          alarm(connectionParameters.timeOut);  
          alarm_stop=0;

          state=0;
          res=0;
          res_old=0;
      
          while(alarm_stop==0 && state!=5) 
          {
            res+=read(fd, &UA_recieve, 1);
            if(res<res_old) {printf("\n\n----------------ERROR: read() failed\n"); error_read++; return-1;}
            res_old=res;
              
            state=UA_state_machine(state, UA_recieve);
          }
                
          if(state==5) 
          {
            alarm(0);
            //printf("\n\n----------------GREAT SUCCESS: Connection opened, recieved UA correctly\n"); 
            return 1;
          }
          attempt++;
      }
  }
  else if(connectionParameters.role==RECEIVER) 
  {
      unsigned char SET_recieve, UA_send[5];
      res=0; res_old=0; state=0; 

      while(state!=5) 
      {
          res+=read(fd, &SET_recieve, 1);
          if(res<res_old) {printf("\n\n----------------ERROR: read() failed\n"); error_read++; return-1;}
          res_old=res;

          state=SET_state_machine(state, SET_recieve);
      }

      if(state==5)
      {
          UA_send[0] = FLAG;
          UA_send[1] = A;
          UA_send[2] = UA_C; 
          UA_send[3] = A^UA_C;
          UA_send[4] = FLAG;

          res = write(fd,UA_send,5); 
          if(res<0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
          return 1;
      }
    }
    else {printf("\n\n----------------ERROR: Wrong role\n"); return-1;}
}

int I_state_machine(int state, unsigned char pos) 
{
  switch (state)
  {
  case 0:
    if(pos==FLAG) state=1;
    break;

  case 1:
    if(pos==A) state=2;
    break;
  
  case 2:
    if(pos==SET_C) state=30;
    else if((pos==I_C0 && sequence_number_reciever==0) || (pos==I_C1 && sequence_number_reciever==1)) state=3;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 3:
    if((pos==A^I_C0 && sequence_number_reciever==0) || (pos==A^I_C1 && sequence_number_reciever==1)) state=4;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 30:
    if(pos==A^SET_C) state=40;
    else if(pos==FLAG) state=1;
    else state=0;

  case 4: 
    if(pos==FLAG) state=5;
    break;

  case 40:
    if(pos==FLAG) state=50;
    else if(pos==FLAG) state=1;
    else state=0;
    
  case 5:
    break;
  
  case 50:
    if(pos==FLAG) state=1;
    else state=0;
    break;
  }

  return state;
}

int RR_REJ_state_machine(int state, unsigned char pos)
{
  switch (state)
  {
  case 0:
    if(pos==FLAG) state=1;
    break;

  case 1:
    if(pos==A) state=2;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 2:
    if((pos==RR_C0 && sequence_number_transmitter==0) || (pos==RR_C1 && sequence_number_transmitter==1)) state=3;
    else if((pos==REJ_C0 && sequence_number_transmitter==1) || (pos==REJ_C1 && sequence_number_transmitter==0)) state=30;
    else if(pos==FLAG) state=1;
    else state=0;
    break;
  
  case 3:
    if((pos==A^RR_C0 && sequence_number_transmitter==0) || (pos==A^RR_C1 && sequence_number_transmitter==1)) state=4;
    else if(pos==FLAG) state=1;
    else state=0;
    break;

  case 30:
    if((pos==A^REJ_C0 && sequence_number_transmitter==1) || (pos==A^REJ_C1 && sequence_number_transmitter==0)) state=40;
    else if(pos==FLAG) state=1;
    else state=0;
    break;

  case 4:
    if(pos==FLAG) state=5;
    else state=0;
    break;
  
  case 40:
    if(pos==FLAG) state=50;
    else state=0;
    break;

  case 5:
    break;
  
  case 50:
    break;
  }

  return state;
}

void stuffing(unsigned char buf, unsigned char *I_send, int *j)
{
  if(buf==FLAG)
  {
    I_send[*j]=ESC1;
    I_send[*j+1]=ESC2;
    *j+=2;
  } 
  else if(buf==ESC1)
  {
    I_send[*j]=buf;
    I_send[*j+1]=ESC3;
    *j+=2;
  }
  else
  {
    I_send[*j]=buf;
    *j+=1;
  }
}

int DE_stuffing(unsigned char* buf, int I_recieve_size, unsigned char* buf_DE_stuffed)
{
  int j=0;

  for(int i=4; i<I_recieve_size-1; i++) //-1 so not to DE FLAG
  {
    if(buf[i]==ESC1 && buf[i+1]==ESC2)
    {
      buf_DE_stuffed[j]=FLAG;
      i++;
    }
    else if(buf[i]==ESC1 && buf[i+1]==ESC3)
    {
      buf_DE_stuffed[j]=ESC1;
      i++;
    }
    else 
    {
      buf_DE_stuffed[j]=buf[i];
    }
    j++;
  }

  return j; //j=pos next=total
}

int BCC2_calculate(unsigned char* buf, int bufSize, unsigned char *I_send)
{
  unsigned char BCC2=0x00;
  int j=4;
  
  for(int i=0; i<bufSize; i++)
  {
    BCC2=BCC2^buf[i];
    stuffing(buf[i], I_send, &j);
  }

  stuffing(BCC2, I_send, &j);

  return j; //j=pos next=size
}

int BCC2_verify(unsigned char* buf, int DataBCC2_DE_size)
{
  unsigned char BCC2_verify=0x00;
  
  for(int i=0; i<DataBCC2_DE_size-1; i++) //-1 so not check BCC2
  {
    BCC2_verify=BCC2_verify^buf[i];
  }

  if(BCC2_verify==buf[DataBCC2_DE_size-1]) {/*printf("\nGOOD\n");*/ return 1;}
  else {/*printf("\nNOT GOOD\n");*/ return -1;}
}

int llwrite(char* buf, int bufSize)
{
  if(bufSize>MAX_PAYLOAD_SIZE) return -1;

  unsigned char *I_send, RR_REJ_recieve;
  I_send=(unsigned char *)malloc(((bufSize+1)*2+5)*sizeof(unsigned char)); //+1 for BCC2, *2 for stuffing, +5 for header+flag
  if(I_send==NULL) return -1;

  int I_send_stuffed_size=0, res=0, res_old=0, attempt=0, state=0, j=0;

  I_send[0]=FLAG;
  I_send[1]=A;

  //printf("\n----------------\nSN_reciever_send=%d\n", sequence_number_transmitter);
  if(sequence_number_transmitter==0) {I_send[2]=I_C0; sequence_number_transmitter=1;}
  else if(sequence_number_transmitter==1) {I_send[2]=I_C1; sequence_number_transmitter=0;}
  I_send[3]=A^I_send[2];

  I_send_stuffed_size=BCC2_calculate(buf, bufSize, I_send);

  //printf("I_send_stuffed_size=%d\n", I_send_stuffed_size);
  I_send[I_send_stuffed_size]=FLAG;
  I_send_stuffed_size++;
  //printf("bufSize=%d\n", bufSize);
  //printf("I_send_tuffed_size=%d\n", I_send_stuffed_size);

  I_send=(unsigned char *)realloc(I_send, I_send_stuffed_size);
  if(I_send==NULL) return -1;

  PACKAGE_NUM_SEND++;

  (void) signal(SIGALRM, atende);  
  conta=1;
  attempt=0;
  
  while(attempt<num_tries)
  {
    //printf("-------------------------------------------Attempt=%d\n", attempt);
    res = write(fd, I_send, I_send_stuffed_size);   
    if(res<0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
    //printf("PACKAGE_NUM_SEND=%d\n", PACKAGE_NUM_SEND);
    //printf("Attempt=%d\n", attempt);

    alarm(time_out);  
    alarm_stop=0;

    state=0;
    j=0;   
    res=0;
    res_old=0;

    while(alarm_stop==0 && state!=5 && state!=50) 
    {
      res+=read(fd, &RR_REJ_recieve, 1);
      if(res<res_old) {printf("\n\n----------------ERROR: read() failed\n"); error_read++; return-1;}
      res_old=res;

      //printf("\n\nState_initial: %d\n", state);
      //printf("RR_REJ_recieve[%d]=%x\n", j, RR_REJ_recieve);
      
      state=RR_REJ_state_machine(state, RR_REJ_recieve); //maquina de estados que ve rr ou rej

      //printf("State_final: %d\n", state);

      j++;
    }
    if(state==5) {statNum_RRrecieve++; break;}
    else if(state==50) {/*printf("REJ\n");*/ statNum_REJrecieve++;} 

    attempt++;
    statNum_retransmitted++;
  }

  if(state==50 && sequence_number_transmitter==0) {/*printf("RESEND\n");*/ sequence_number_transmitter=1;}
  else if(state==50 && sequence_number_transmitter==1) {/*printf("RESEND\n");*/ sequence_number_transmitter=0;}
  //printf("SN_reciever_recieve=%d\n", sequence_number_transmitter);
  
  alarm(0);
  return I_send_stuffed_size;
}

int llread(char* packet)
{
  int res=0, res_old=0, state=0, I_recieve_size=0, DataBCC2_DE_size=0;

  unsigned char *I_recieve;
  I_recieve=(unsigned char *)malloc((MAX_PAYLOAD_SIZE+5)*2*sizeof(unsigned char));
  if(I_recieve==NULL) return -1;

  statNum_recieved++;

  //printf("\nState_incial: %d size=%d\n", state, I_recieve_size);
  //printf("\n----------------\n");
  //printf("PACKAGE_NUM_RECIEVE=%d\n", PACKAGE_NUM_RECIEVE);
  while(state!=5) 
  { 
      if(state==0) I_recieve_size=0;
      else if(state==1) I_recieve_size=1;
      
      res+=read(fd, &I_recieve[I_recieve_size], 1);
      if (res>res_old)
      {
        //printf("------------\n");
        //printf("\n\nState_initial: %d\n", state);
        //printf("I_recieve[%d]=%x\n", I_recieve_size, I_recieve[I_recieve_size]);
        state=I_state_machine(state, I_recieve[I_recieve_size]/*, &I_recieve_size,*/);
        //printf("State_final: %d\n", state);

        if(state==50) 
        {
          //printf("\nSEND UA\n");
          unsigned char UA_send[5];
          int res_UA=0;

          UA_send[0] = FLAG;
          UA_send[1] = A;
          UA_send[2] = UA_C; 
          UA_send[3] = A^UA_C;
          UA_send[4] = FLAG;

          res_UA = write(fd,UA_send,5); 
          if(res_UA<0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
        }

        if(I_recieve_size<((MAX_PAYLOAD_SIZE+5)*2)) I_recieve_size++; //in the end=final flag       
      }
      else if(res==res_old) {/*printf(".");*/ statRead_0++;}
      else if(res<res_old) {printf("\n\n----------------ERROR: read() failed\n"); error_read++; return-1;}

      res_old=res;
  }
  //printf("\n");
  //printf("State_final: %d size=%d\n", state, I_recieve_size);
  //printf("SN_reciever_inicial=%d\n", sequence_number_reciever);
  //printf("I_recieve_size=%d\n", I_recieve_size);
  
  I_recieve=(unsigned char *)realloc(I_recieve, I_recieve_size);
  if(I_recieve==NULL) return -1;

  unsigned char *DataBCC2_DE, RR_REJ_send[5]={};
  DataBCC2_DE=(unsigned char *)malloc((I_recieve_size-4)*sizeof(unsigned char));
  if(DataBCC2_DE==NULL) return -1;

  DataBCC2_DE_size=DE_stuffing(I_recieve, I_recieve_size, DataBCC2_DE); // I_size_DE is total not pos
  
  DataBCC2_DE=(unsigned char *)realloc(DataBCC2_DE, DataBCC2_DE_size);
  if(DataBCC2_DE==NULL) return -1;

  //BCC2_verify(DataBCC2_DE, DataBCC2_DE_size);
  
  if(BCC2_verify(DataBCC2_DE, DataBCC2_DE_size)==-1) //dont save data, flip sequence number, send REJ
  {
    //printf("SEND REJ\n");
    //printf("SN_reciever_recieve=%d\n", sequence_number_reciever);

    RR_REJ_send[0]=FLAG;
    RR_REJ_send[1]=A;

    if(sequence_number_reciever==0) RR_REJ_send[2]=REJ_C0; 
    else RR_REJ_send[2]=REJ_C1;

    RR_REJ_send[3]=A^RR_REJ_send[2];
    RR_REJ_send[4]=FLAG;
    
    res = write(fd,RR_REJ_send,5);    
    if(res<=0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
    statNum_REJsend++;
    
    return 0; 
  }
  else //check sequence number, if new (save data, flip sequence number), send RR
  {
    //printf("SEND RR\n");
    PACKAGE_NUM_RECIEVE++;
    //printf("SN_reciever_recieve=%d\n", sequence_number_reciever);

    if(sequence_number_reciever!=sequence_number_old_reciever) // if new save data flip sequence number
    {
        for(int i=0; i<DataBCC2_DE_size-1; i++) packet[i]=DataBCC2_DE[i];
        sequence_number_old_reciever=sequence_number_reciever;

        if(sequence_number_reciever==0) sequence_number_reciever=1;
        else sequence_number_reciever=0;
    }
    else {/*printf("DUPLICATE\n");*/ DataBCC2_DE_size=1;} // if not new dont save flip sequence number return 0

    RR_REJ_send[0]=FLAG;
    RR_REJ_send[1]=A;

    if(sequence_number_reciever==0) RR_REJ_send[2]=RR_C0; 
    else  RR_REJ_send[2]=RR_C1;

    RR_REJ_send[3]=A^RR_REJ_send[2];
    RR_REJ_send[4]=FLAG;
    
    res = write(fd,RR_REJ_send,5);    
    if(res<0) {printf("\n\n----------------ERROR: write() failed\n"); error_write++; return-1;}
    
    statNum_RRsend++;

    return (DataBCC2_DE_size-1); //-1 cuz BCC2
  }

}

int DISC_state_machine(int state, unsigned char pos)
{
  switch (state)
  {
  case 0:
    if(pos==FLAG)state=1;
    break;

  case 1:
    if(pos==A) state=2;
    else state=1;
    break;

  case 2:
    if(pos==C_DISC) state=3;
    else state=0;
    break;

  case 3:
    if(pos==A^C_DISC) state=4;
    else state=0;
    break;

  case 4:
    if(pos==FLAG) state=5;
    else state=0;
    break;

  case 5:
    break;
  }

  return state;
}

int llclose(int showStatistics)
{  
  int state=0, res=0, attempt=0;
  unsigned char DISC_send[5]={}, DISC_recieve, UA_send[5]={}, UA_recieve;
  
  DISC_send[0]=FLAG;
  DISC_send[1]=A;
  DISC_send[2]=C_DISC;
  DISC_send[3]=A^C_DISC;
  DISC_send[4]=FLAG;

  if(role==TRANSMITTER)
  {
    if(showStatistics==1)
    {
      printf("statNum_retransmitted=%d\n", statNum_retransmitted);
      printf("statNum_timeOut=%d\n", statNum_timeOut);
      printf("statNum_RRrecieve=%d\n", statNum_RRrecieve);
      printf("statNum_REJrecieve=%d\n", statNum_REJrecieve);
      printf("error_read=%d\n", error_read);
      printf("error_write=%d\n", error_write);
      printf("statRead_0=%d\n", statRead_0);
    }

    while(attempt<num_tries)
    {
      res=write(fd, DISC_send, 5); 
      if(res<=0) {error_write++; return -1;}

      alarm(time_out);  
      alarm_stop=0;

      state=0;
      res=0;

      while(alarm_stop==0 && state!=5)
      {
        res=read(fd, &DISC_recieve, 1);
        if(res>0) state=DISC_state_machine(state, DISC_recieve);
        else if(res<0) error_read++;
      }


      if(state==5)
      {
        alarm(0);
        UA_send[0] = FLAG;
        UA_send[1] = A;
        UA_send[2] = UA_C; 
        UA_send[3] = A^UA_C;
        UA_send[4] = FLAG;

        res = write(fd,UA_send,5); 
        if(res<=0) {error_write++; return -1;}
        return 1;
      }
      attempt++;
    }

    return -1;
  }
  else if(role==RECEIVER)
  {
    if(showStatistics==1)
    {
      printf("statNum_recieved=%d\n", statNum_recieved);
      printf("statNum_RRsend=%d\n", statNum_RRsend);
      printf("statNum_REJsend=%d\n", statNum_REJsend);
      printf("error_read=%d\n", error_read);
      printf("error_write=%d\n", error_write);
      printf("statRead_0=%d\n", statRead_0);
    }

    while(state!=5)
    {
      res=read(fd, &DISC_recieve, 1);
      if(res>0) state=DISC_state_machine(state, DISC_recieve);
      else if(res==0) statRead_0++;
      else if(res<0) error_read++; 
    }

    res=write(fd, DISC_send, 5); 

    if(res<=0) {error_write++; return -1;}

    state=0;
    while(state!=5)
    {
      res=read(fd, &UA_recieve, 1);
      if(res>0) state=UA_state_machine(state, UA_recieve);
      if(res==0) statRead_0++;
      if(res==-1) error_read++;
      
    }

    return 1;
  }

  return -1;
}