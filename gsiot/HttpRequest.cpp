#pragma once
// HTTPrequest.cpp: implementation of the Request class.   
//   
//////////////////////////////////////////////////////////////////////   

//#include "HTTPrequest.h"
#include "HttpRequest.h"

//////////////////////////////////////////////////////////////////////   
// Construction/Destruction   
//////////////////////////////////////////////////////////////////////   
       
Request::Request()   
{   
       
}   
       
Request::~Request()   
{   
       
}   


char *strupr(char *str)  //jyc20170318 add for linux
{
	char *ptr = str;

	while (*ptr != '\0 ') {
		if (islower(*ptr))
		*ptr = toupper(*ptr);
		ptr++;
	}
	return str;
}
       
//*******************************************************************************************************   
//MemBufferCreate:    
//                  Passed a MemBuffer structure, will allocate a memory buffer    
//                   of MEM_BUFFER_SIZE.  This buffer can then grow as needed.   
//*******************************************************************************************************   
void Request::MemBufferCreate(MemBuffer *b)   
{   
    b->size = MEM_BUFFER_SIZE;   
    b->buffer =(unsigned char *) malloc( b->size );   
    b->position = b->buffer;   
}   
       
//*******************************************************************************************************   
// MemBufferGrow:     
//                  Double the size of the buffer that was passed to this function.    
//*******************************************************************************************************   
void Request::MemBufferGrow(MemBuffer *b)   
{   
    size_t sz;   
    sz = b->position - b->buffer;   
    b->size = b->size *2;   
    b->buffer =(unsigned char *) realloc(b->buffer,b->size);   
    b->position = b->buffer + sz; // readjust current position   
}   
       
//*******************************************************************************************************   
// MemBufferAddByte:    
//                  Add a single byte to the memory buffer, grow if needed.   
//*******************************************************************************************************   
void Request::MemBufferAddByte(MemBuffer *b,unsigned char byt)   
{   
    if( (size_t)(b->position-b->buffer) >= b->size )   
        MemBufferGrow(b);   
       
    *(b->position++) = byt;   
}   
       
//*******************************************************************************************************   
// MemBufferAddBuffer:   
//                  Add a range of bytes to the memory buffer, grow if needed.   
//*******************************************************************************************************   
void Request::MemBufferAddBuffer(MemBuffer *b,   
                    unsigned char *buffer, size_t size)   
{   
    while( ((size_t)(b->position-b->buffer)+size) >= b->size )   
        MemBufferGrow(b);   
       
    memcpy(b->position,buffer,size);   
    b->position+=size;   
}   
       
//*******************************************************************************************************   
// GetHostAddress:    
//                  Resolve using DNS or similar(WINS,etc) the IP    
//                   address for a domain name such as www.wdj.com.    
//*******************************************************************************************************   
DWORD Request::GetHostAddress(LPCSTR host)  
{   
    struct hostent *phe;   
    char *p;   
       
    phe = gethostbyname( host );   
                   
    if(phe==NULL)   
        return 0;        
    p = *phe->h_addr_list;   
    return *((DWORD*)p);   
}  

      
//*******************************************************************************************************   
// SendString:    
//                  Send a string(null terminated) over the specified socket.   
//*******************************************************************************************************   
void Request::SendString(SOCKET sock,LPCSTR str)   
{   
    send(sock,str,strlen(str),0);
}   
       
//*******************************************************************************************************   
// ValidHostChar:    
//                  Return TRUE if the specified character is valid   
//                      for a host name, i.e. A-Z or 0-9 or -.:    
//*******************************************************************************************************   
BOOL Request::ValidHostChar(char ch)   
{   
    return( isalpha(ch) || isdigit(ch)   
        || ch=='-' || ch=='.' || ch==':' );   
}   
       
       
//*******************************************************************************************************   
// ParseURL:    
//                  Used to break apart a URL such as    
//                      http://www.localhost.com:80/TestPost.htm into protocol, port, host and request.   
//*******************************************************************************************************   
void Request::ParseURL(LPCSTR url,LPSTR protocol,int lprotocol,   
        LPSTR host,int lhost,LPSTR request,int lrequest,int *port)   
{   
    char *work,*ptr,*ptr2;   
       
    *protocol = *host = *request = 0;   
    *port=80;   
       
    work = strdup(url);   
    strupr(work);   
       
    ptr = strchr(work,':');                         // find protocol if any   
    if(ptr!=NULL)   
    {   
        *(ptr++) = 0;   
        //lstrcpynA(protocol,work,lprotocol);   //jyc20170319 modify
		strncpy(protocol,work,lprotocol);
    }   
    else   
    {   
        //lstrcpynA(protocol,"HTTP",lprotocol); 
		strncpy(protocol,"HTTP",lprotocol);
        ptr = work;   
    }   
       
    if( (*ptr=='/') && (*(ptr+1)=='/') )            // skip past opening /'s    
        ptr+=2;   
       
    ptr2 = ptr;                                     // find host   
    while( ValidHostChar(*ptr2) && *ptr2 )   
        ptr2++;   
       
    *ptr2=0;   
    //lstrcpynA(host,ptr,lhost); 
	strncpy(host,ptr,lhost);
       
    //lstrcpynA(request,url + (ptr2-work),lrequest);   // find the request  
	strncpy(request,url + (ptr2-work),lrequest);
       
    ptr = strchr(host,':');                         // find the port number, if any   
    if(ptr!=NULL)   
    {   
        *ptr=0;   
        *port = atoi(ptr+1);   
    }   
       
    free(work);   
}  
       
//*******************************************************************************************************   
// SendHTTP:    
//                  Main entry point for this code.     
//                    url           - The URL to GET/POST to/from.   
//                    headerSend        - Headers to be sent to the server.   
//                    post          - Data to be posted to the server, NULL if GET.   
//                    postLength    - Length of data to post.   
//                    req           - Contains the message and headerSend sent by the server.   
//   
//                    returns 1 on failure, 0 on success.   
//*******************************************************************************************************   
int Request::SendHTTP(LPCSTR url,
                      LPCSTR headerReceive,
                      BYTE *post,   
    				  DWORD postLength,
                      HTTPRequest *req)   
{   
    //WSADATA         WsaData;   //jyc20170318 remove
    SOCKADDR_IN     sin;   
    SOCKET          sock;   
    char            buffer[512];   
    //char            protocol[20],host[256],request[1024];   
    int             l,port,chars,err;   
    MemBuffer       headersBuffer,messageBuffer;   
    char            headerSend[1024];   
    BOOL            done;   
	
	/*
	ParseURL(url,protocol,sizeof(protocol),host,sizeof(host),       // Parse the URL   
        request,sizeof(request),&port);   
	if(strcmp(protocol,"HTTP"))   
        return 1;   
    */   
	char protocol[20]={"HTTP"};  //jyc20170321 modify  parseurl have trouble
	char host[256]={"API.GSSS.CN"};
	char request[1024]={"/iot_ctl_update.ashx?getupdate=box_update"};//{"/iot_ctl_update.ashx"}; //
	port = 80;
    sock = socket (AF_INET, SOCK_STREAM, 0);      
    if (sock == INVALID_SOCKET)   
        return 1; 
    sin.sin_family = AF_INET;                                       //Connect to web sever  
	sin.sin_port = htons( (unsigned short)port );  
	printf("host =%s...\n",host); //jyc20170320 debug
    sin.sin_addr.s_addr = GetHostAddress(host); 	//jyc20170320 sometimes have trouble
	//jyc20170318 modify for linux
    //if( connect (sock,(LPSOCKADDR)&sin, sizeof(SOCKADDR_IN) ) )   
    //{          
    //    return 1;   
    //}    
	
	if(-1 == connect(sock,(struct sockaddr*)&sin,sizeof(struct sockaddr)))  //linux
    {
        return 1;
    }
    if( !*request ){   
        //lstrcpynA(request,"/",sizeof(request));  //
		strncpy(request,"/",sizeof(request)); //cannot do this have trouble memory bad
	}    
    if( post == NULL ) {    
        strcpy(headerSend, "GET ");   
    }   
    else{ 
        strcpy(headerSend, "POST ");   
    }     
    strcat(headerSend, request);   
    strcat(headerSend, " HTTP/1.0\r\n");
	strcat(headerSend,"Accept: */*\r\n");
	strcat(headerSend, "User-Agent: Mozilla/4.0\r\n");      
    if(postLength){   
        //wsprintfA(buffer,"Content-Length: %ld\r\n",postLength); //jyc20170318 modify
		sprintf(buffer,"Content-Length: %ld\r\n",postLength);
		strcat(headerSend, buffer);   
    }   
    strcat(headerSend, "Host: ");       
    strcat(headerSend, host);   
    strcat(headerSend, "\r\n");   
    if( (headerReceive!=NULL) && *headerReceive ){   
        strcat(headerSend, headerReceive);   
    }           
    strcat(headerSend, "\r\n"); 
	
	// SEND HEADER ALL
	LOGMSG("%s", headerSend );
	SendString( sock, headerSend );
    if( (post!=NULL) && postLength )   
    {   
        send(sock,(const char*)post,postLength,0);   
        post[postLength]    = '\0';   
               
        strcat(headerSend, (const char*)post);   
    }  
	req->headerSend      = (char*) malloc( sizeof(char*) * strlen(headerSend));   
	strcpy(req->headerSend, (char*) headerSend ); 
	MemBufferCreate(&headersBuffer );   
    chars = 0;   
    done = false; //done = FALSE;   //jyc20170318 FALSE ->false  TRUE->true 
	while(!done){   
        l = recv(sock,buffer,1,0);   
        if(l<0)done=true;   
       
        switch(*buffer){   
            case '\r':   
                break;   
            case '\n':   
                if(chars==0)done = true;   
                chars=0;   
                break;   
            default:   
                chars++;   
                break;   
        }     	 
        MemBufferAddByte(&headersBuffer,*buffer);   
    }   
	req->headerReceive   = (char*) headersBuffer.buffer;   //jyc20170320 remove
    *(headersBuffer.position) = 0; 
	MemBufferCreate(&messageBuffer);                            // Now read the HTTP body    
    do   
    {   
        l = recv(sock,buffer,sizeof(buffer)-1,0);   
        if(l<0)   
            break;   
        *(buffer+l)=0;   
        MemBufferAddBuffer(&messageBuffer, (unsigned char*)&buffer, l);   
    } while(l>0); 
	*messageBuffer.position = 0; 
	req->message = (char*) messageBuffer.buffer;   
    req->messageLength = (messageBuffer.position - messageBuffer.buffer);   
	//closesocket(sock);                                          // Cleanup 
	close(sock); //jyc20170318 modify
    return 0;   
}   
       
       
//*******************************************************************************************************   
// SendRequest   
//   
//*******************************************************************************************************   
bool Request::SendRequest(bool IsPost, const char* url, std::string &psHeaderSend, 
                          std::string &psHeaderReceive, std::string &psMessage) 
{   
    HTTPRequest         req;   
    int                 i,rtn;
    LPSTR               buffer;   
       
    req.headerSend                          = NULL;   
    req.headerReceive                       = NULL;   
    req.message                             = NULL;  
    //Read in arguments             
    if(IsPost)   
    {                                                    /* POST */   
		i       = psHeaderSend.size();   
        buffer  = (char*) malloc(i+1);   
        strcpy(buffer, psHeaderSend.c_str());         
        rtn             = SendHTTP( url,"Content-Type: application/x-www-form-urlencoded\r\n",   
                                    (unsigned char*)buffer,i,&req);               
        free(buffer);  
    }   
    else                                                /* GET */   
        rtn = SendHTTP(url,NULL,NULL,0,&req);  
	if(!rtn)                                            //Output message and/or headerSend    
    {   
        psHeaderSend        = req.headerSend;   
        psHeaderReceive     = req.headerReceive;   
        psMessage           = req.message;   
   		free(req.headerSend); 
		free(req.headerReceive); 
		free(req.message); 
		return true;
    }
	return false;
}
