//FEUP 2021/2022 - Licenciatura em Engenharia Electrotécnica e de Computadores - Redes de Computadores
//Duarte Ribeiro Afonso Branco                  up201905327
//Pedro Afonso da Silva Correia de Castro Lopes up201907097

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>

#define MAX_STRING_SIZE 1024
#define DEFAULT_FTP_PORT 21

struct URL_struct
{
    char* user;
    char* pass;
    char* host;
    char* path;
};

int processURL(char* url, struct URL_struct* URL)
{
    printf("----------------processURL\n");

    //ftp://<user>:<password>@<host>/<url-path>
    puts(url);
    if(url[0]!='f' || url[1]!='t' || url[2]!='p' || url[3]!=':' || url[4]!='/' || url[5]!='/') return -1;
    //printf("1\n");

    int i=6, state=0, pos=0, url_lenght=strlen(url);
    printf("url_lenght=%d\n", url_lenght);

    //if no user/pass set as anonymous
    const char ch1='@';
    if(strchr(url, ch1)==NULL) //ver caso ftp://abc:asd/asdfg, posso ter : sem ser naquele sitio?
    {
        state=2;
        strcpy(URL->user, "anonymous");
        strcpy(URL->pass, "anonymous");
    }

    while(i<url_lenght)
    {
        //printf("url[%d]=%c\n", i, url[i]);
        switch (state)
        {
            //user until : 
            case 0:
                if(pos>MAX_STRING_SIZE-1) return -1;
                else if(url[i]==':')
                {
                    URL->user[pos]='\0';
                    pos=0;
                    state=1;
                }
                else
                {
                    URL->user[pos]=url[i];
                    pos++;
                }
                break;

            //pass until @
            case 1:
                if(pos>MAX_STRING_SIZE-1) return -1;
                else if(url[i]=='@')
                {
                    URL->pass[pos]='\0';
                    pos=0;
                    state=2;
                }
                else
                {
                    URL->pass[pos]=url[i];
                    pos++;
                }
                break;

            //host until /
            case 2:
                if(pos>MAX_STRING_SIZE-1) return -1;
                else if(url[i]=='/')
                {
                    URL->host[pos]='\0';
                    pos=0;
                    state=3;
                }
                else
                {
                    URL->host[pos]=url[i];
                    pos++;
                }
                break;
            
             //path until not i<url_lenght
             case 3:
                if(pos>MAX_STRING_SIZE-1) return -1;
                else
                {
                    URL->path[pos]=url[i];
                    pos++;
                }
                break;

            default:
                return -1;
                break;
        }
        i++;
    }

    /*printf("user:%s\n", URL->user);
    printf("pass:%s\n", URL->pass);
    printf("host:%s\n", URL->host);
    printf("path:%s\n", URL->path);*/
    URL->path[pos]='\0';
    if(state!=3 || pos==0) return -1;

    return 0;
}

int get_file_name(char* path, char* file_name)
{
    printf("----------------get_file_name\n");
    const char ch1='/';
    if(strrchr(path, ch1)==NULL) strcpy(file_name, path);
    else strcpy(file_name, strrchr(path, ch1)+1);
    

    printf("strlen(file_name)=%ld\n", strlen(file_name));
    if(strlen(file_name)==0) return -1;
    else return 0;
}

int read_reply(int sockfd, char* reply_code)
{
    printf("----------------read_reply\n");

    int i=0, res=0, state=0, done=0;
    char c;
    
    while(done!=1)
    {
        res+=read(sockfd, &c, 1);
        //else if()
        printf("%c", c);
        switch (state)
        {
            case 0:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=1;
                }
                break;

            case 1:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=2;
                }
                else
                {
                    i=0;
                    state=0;
                }
                break;

            case 2:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=3;
                }
                else
                {
                    i=0;
                    state=0;
                }
                break;
            case 3: //if read 3 digits, if - go to state 4 and ignore everything until new line, else done in \n
                if(c=='-') state=4;
                else state=5;
                break;
            case 4:
                if(c=='\n')
                { 
                    i=0;
                    state=0;
                }
                break;
            case 5:
                if(c=='\n') done=1;
                break;
            
            default:
                return -1;
                break;
    
        }
    }
    //printf("strlen(reply_code):%ld\n", strlen(reply_code));
    //printf("reply_code:%s\n", reply_code);

    return res;
}

int read_reply_pasv(int sockfd, char* reply_code, char* num1, char* num2)
{
    printf("----------------read_reply\n");

    int i=0, j=0, k=0, res=0, state=0, done=0;
    char c;
    
    while(done!=1)
    {
        res+=read(sockfd, &c, 1);
        //else if()
        printf("%c", c);
        switch (state)
        {
            case 0:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=1;
                }
                break;

            case 1:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=2;
                }
                else
                {
                    i=0;
                    state=0;
                }
                break;

            case 2:
                if(c>=48 && c<=57)
                {
                    reply_code[i]=c;
                    i++;
                    state=3;
                }
                else
                {
                    i=0;
                    state=0;
                }
                break;
            case 3: //if read 3 digits, if 4x ',' go to state 4 
                if(c==',') j++;

                if(j==4) state=4;
                break;
            case 4:
                if(c>=48 && c<=57)
                {
                    num1[k]=c;
                    k++;
                } 
                else if(c==',')
                {
                    num1[k]='\0';
                    k=0;
                    state=5;
                } 

                break;
            case 5:
                if(c>=48 && c<=57)
                {
                    num2[k]=c;
                    k++;
                } 
                else if(c=='\n') 
                {
                    num2[k]='\0';
                    done=1;
                }
                break;
            
            default:
                return -1;
                break;
    
        }
    }
    //printf("strlen(reply_code):%ld\n", strlen(reply_code));
    //printf("reply_code:%s\n", reply_code);
    //puts(num1);
    //puts(num2);
    return res;
}

int send_msg(int sockfd, char* type, char* content)
{
    printf("----------------send_msg\n");
    int res=0, size=0;

    printf("type: %s | content: %s\n", type, content);

    if(content!=NULL) size=strlen(type)+strlen(content)+2+1;
    else size=strlen(type)+2+1;

    printf("size:%d\n", size);
    
    //size: +2 for \r\n, +1 for \0. \0 done by strcat.
    char* msg=(char*)calloc(size, sizeof(char)); if(msg==NULL) {{printf("----------------ERROR: Malloc user\n"); return -1;}}
 

    strcpy(msg, type);
    if(content!=NULL) strcat(msg, content);
    strcat(msg, "\r\n");

    printf("msg: %s | strlen(msg)=%ld\n", msg, strlen(msg));

    res=write(sockfd, msg, strlen(msg));

    return res;
}

int main(int argc, char* argv[])
{
    if(argc!=2)
    {
        printf("\n----------------ERROR: Please pass URL in agurment in format ftp://<user>:<password>@<host>/<url-path> \n");
        return -1;
    }

    struct URL_struct URL;
    URL.user=(char*)malloc(MAX_STRING_SIZE*sizeof(char)); if(URL.user==NULL) {printf("----------------ERROR: Malloc user\n"); return -1;}
    URL.pass=(char*)malloc(MAX_STRING_SIZE*sizeof(char)); if(URL.pass==NULL) {printf("----------------ERROR: Malloc pass\n"); return -1;}
    URL.host=(char*)malloc(MAX_STRING_SIZE*sizeof(char)); if(URL.host==NULL) {printf("----------------ERROR: Malloc host\n"); return -1;}
    URL.path=(char*)malloc(MAX_STRING_SIZE*sizeof(char)); if(URL.path==NULL) {printf("----------------ERROR: Malloc path\n"); return -1;}

    puts(argv[1]);

    if(processURL(argv[1], &URL)!=0)
    {
        printf("\n----------------ERROR: Please pass valid URL in agurment in format ftp://<user>:<password>@<host>/<url-path> \n");
        return -1;
    }

                                                    //+1 for \0
    URL.user=(char*)realloc(URL.user, strlen(URL.user)+1); if(URL.user==NULL) {printf("----------------ERROR: Realloc user\n"); return -1;}
    URL.pass=(char*)realloc(URL.pass, strlen(URL.pass)+1); if(URL.pass==NULL) {printf("----------------ERROR: Realloc pass\n"); return -1;}
    URL.host=(char*)realloc(URL.host, strlen(URL.host)+1); if(URL.host==NULL) {printf("----------------ERROR: Realloc host\n"); return -1;}
    URL.path=(char*)realloc(URL.path, strlen(URL.path)+1); if(URL.path==NULL) {printf("----------------ERROR: Realloc path\n"); return -1;}

    printf("URL.user:%s | strlen(URL.user)=%ld\n", URL.user, strlen(URL.user));
    printf("URL.pass:%s | strlen(URL.pass)=%ld\n", URL.pass, strlen(URL.user));
    printf("URL.host:%s | strlen(URL.host)=%ld\n", URL.host, strlen(URL.user));
    printf("URL.path:%s | strlen(URL.path)=%ld\n", URL.path, strlen(URL.user));

    //get ip
    struct hostent *h;
    /*
        struct hostent {
            char    *h_name;	Official name of the host. 
                char    **h_aliases;	A NULL-terminated array of alternate names for the host. 
            int     h_addrtype;	The type of address being returned; usually AF_INET.
                int     h_length;	The length of the address in bytes.
            char    **h_addr_list;	A zero-terminated array of network addresses for the host. 
                        Host addresses are in Network Byte Order. 
        };
        
        #define h_addr h_addr_list[0]	The first address in h_addr_list. 
    */

    h=gethostbyname(URL.host);
    if(h==NULL)
    {
        printf("\n----------------ERROR: gethostname\n");
        return -1;
    }

    printf("h->h_name: %s\n", h->h_name);
    printf("h->h_addr: %s\n", inet_ntoa(*((struct in_addr *)h->h_addr))); //inet_ntoa() function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation
    
    //open connection

    int	sockfd;
	struct	sockaddr_in server_addr;
    /*
        struct sockaddr_in {
            sa_family_t    sin_family; address family: always AF_INET 
            in_port_t      sin_port;   port in network byte order 
            struct in_addr sin_addr;    internet address 
        };

           /Internet address 
           struct in_addr {
               uint32_t       s_addr;     address in network byte order 
            };
    */
	
	//server address handling
    //AF_INET is IPv4 protocols
    //htonl() function converts the unsigned integer hostlong from host byte order to network byte order.

	bzero((char*)&server_addr, sizeof(server_addr)); // delete sizeof(server_adrr) bytes in pointer &server_addr
	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr)));	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(DEFAULT_FTP_PORT);		//server TCP port must be network byte ordered. 
    
	//open an TCP socket
    //socket() creates an endpoint for communication and returns a file descriptor 
    //int socket(int domain, int type, int protocol);
    //The domain argument specifies a communication domain; this selects the protocol family. AF_INET is IPv4 protocols
    //SOCK_STREAM provides sequenced, reliable, two-way, connection-based byte streams
    //The protocol specifies a particular protocol to be used with the socket.  Normally only a single protocol exists to support a particular socket type within a given protocol family, in which case protocol can be specified as 0.

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {printf("----------------ERROR: socket\n"); close(sockfd); return -1;}
	
    //connect to the server
    //int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    //The connect() system call connects the socket referred to by the file descriptor sockfd to the address specified by addr.
    /*
        struct sockaddr {
            sa_family_t sa_family;
            char        sa_data[14];
        }~
    */

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {printf("----------------ERROR: connect\n"); close(sockfd); return -1;}


    //https://en.wikipedia.org/wiki/List_of_FTP_server_return_codes
    
    int res=0;
    char reply_code1[4]=""; //4 for \0, compiler does it, https://stackoverflow.com/questions/8202897/null-terminated-string-in-c

    res=read_reply(sockfd, reply_code1);
    if(res<0) {printf("----------------ERROR: read_reply\n"); close(sockfd); return -1;}
    printf("reply_code1:%s\n", reply_code1);
    if(strcmp(reply_code1, "220\0")!=0) {printf("----------------ERROR: 220 not recieved\n"); close(sockfd); return -1;}

    
    
    //ALL SENT AND RECIEVED END IN \r\n
    //send user

    res=send_msg(sockfd, "user ", URL.user);
    if(res<0) {printf("----------------ERROR: send_msg\n"); close(sockfd); return -1;}
    printf("Written bytes: %d\n", res);

    char reply_code2[4]=""; 
    res=read_reply(sockfd, reply_code2);
    if(res<0) {printf("----------------ERROR: read_reply\n"); close(sockfd); return -1;}

    printf("reply_code2:%s\n", reply_code2);
    if(strcmp(reply_code2, "331\0")!=0) {printf("----------------ERROR: 331 not recieved\n"); close(sockfd); return -1;}

    //send pass
    
    res=send_msg(sockfd, "pass ", URL.pass);
    if(res<0) {printf("----------------ERROR: send_msg\n"); close(sockfd); return -1;}
    printf("Written bytes: %d\n", res);

    char reply_code3[4]=""; 
    res=read_reply(sockfd, reply_code3);
    if(res<0) {printf("----------------ERROR: read_reply\n"); close(sockfd); return -1;}

    printf("reply_code3:%s\n", reply_code3);
    if(strcmp(reply_code3, "230\0")!=0) {printf("----------------ERROR: 230 not recieved\n"); close(sockfd); return -1;}
    
    //send pasv
    
    res=send_msg(sockfd, "pasv ", NULL);
    if(res<0) {printf("----------------ERROR: send_msg\n"); close(sockfd); return -1;}
    printf("Written bytes: %d\n", res);

    char reply_code4[4]=""; 
    char* num1, *num2;
    num1=(char*)malloc(4*sizeof(char)); if(num1==NULL) {printf("----------------ERROR: Malloc num1\n"); close(sockfd); return -1;}
    num2=(char*)malloc(4*sizeof(char)); if(num2==NULL) {printf("----------------ERROR: Malloc num2\n"); close(sockfd); return -1;}
    
    res=read_reply_pasv(sockfd, reply_code4, num1, num2);

    num1=(char*)realloc(num1, strlen(num1)+1); if(num1==NULL) {printf("----------------ERROR: Realloc num1\n"); close(sockfd); return -1;}
    num2=(char*)realloc(num2, strlen(num2)+1); if(num2==NULL) {printf("----------------ERROR: Realloc num2\n"); close(sockfd); return -1;}
    
    if(res<0) {printf("----------------ERROR: read_reply\n"); return -1;}

    printf("reply_code4:%s\n", reply_code4);
    if(strcmp(reply_code4, "227\0")!=0) {printf("----------------ERROR: 227 not recieved\n"); return -1;}
    
    printf("num1:%s | strlen(num1)=%ld | atoi(num1)=%d\n", num1, strlen(num1), atoi(num1));
    printf("num2:%s | strlen(num2)=%ld | atoi(num2)=%d\n", num2, strlen(num2), atoi(num2));

    if(strcmp(reply_code4, "227\0")!=0) {printf("----------------ERROR: 227 not recieved\n"); return -1;}
    
    int new_port=atoi(num1)*256+atoi(num2);
    printf("port_client:%d\n", new_port);

    //connect client to new_port

    int	sockfd_new;
	struct	sockaddr_in new_addr;
    
    bzero((char*)&new_addr, sizeof(new_addr)); // delete sizeof(server_adrr) bytes in pointer &server_addr
	new_addr.sin_family = AF_INET; 
	new_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr)));	//32 bit Internet address network byte ordered
	new_addr.sin_port = htons(new_port);		//server TCP port must be network byte ordered. 
    
    if((sockfd_new = socket(AF_INET, SOCK_STREAM, 0)) < 0) {printf("----------------ERROR: socket\n"); close(sockfd); close(sockfd_new); return -1;}
    if(connect(sockfd_new, (struct sockaddr *)&new_addr, sizeof(new_addr)) < 0) {printf("----------------ERROR: connect\n"); close(sockfd); close(sockfd_new); return -1;}
   
    //send retr 
    
    res=send_msg(sockfd, "retr ", URL.path);
    if(res<0) {printf("----------------ERROR: send_msg\n"); return -1;}
    printf("Written bytes: %d\n", res);

    char reply_code5[4]=""; 
    res=read_reply(sockfd, reply_code5);
    if(res<0) {printf("----------------ERROR: read_reply\n"); return -1;}

    printf("reply_code5:%s\n", reply_code5);
    if(strcmp(reply_code5, "150\0")!=0) {printf("----------------ERROR: 150 not recieved\n"); return -1;}
    
    //filename

    char* file_name;
    file_name=(char*) malloc(MAX_STRING_SIZE*sizeof(char)); if(file_name==NULL) {printf("----------------ERROR: Malloc file_name\n"); close(sockfd); close(sockfd_new); return -1;}
    
    if(get_file_name(URL.path, file_name)<0) {printf("----------------ERROR: get_file_name. Please enter valid path with file_name in format path/file_name\n"); return -1;}
    file_name=(char*)realloc(file_name, strlen(file_name)+1); if(file_name==NULL) {printf("----------------ERROR: Realloc file_name.\n"); close(sockfd); close(sockfd_new); return -1;}
    
    printf("file_name:%s\n", file_name);

    //download
    
    FILE* fd;
    fd=fopen(file_name, "w");
    if(fd==NULL)  {printf("----------------ERROR: fopen\n"); close(sockfd); close(sockfd_new); return -1;}

    char* packet;
    packet=(char*) malloc(MAX_STRING_SIZE*sizeof(char)); if(packet==NULL) {printf("----------------ERROR: Malloc packet\n"); close(sockfd); close(sockfd_new); return -1;}

    int fd_size=0;
    res=0;
    while(res=read(sockfd_new, packet, MAX_STRING_SIZE)) fd_size+=fwrite(packet, sizeof(char), res, fd); 

    printf("fd_size:%d\n", fd_size);
    
    close(sockfd);
    close(sockfd_new);
    fclose(fd);

    return 0;
}