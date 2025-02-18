/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-2005 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1997-2004 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: read.c,v 1.49.2.14 2005/02/23 15:41:55 jnelson Exp $*/

#include "boa.h"
#ifdef BOA_WITH_OPENSSL
#include <openssl/ssl.h>
extern int SSL_write(SSL *ssl, const void *buf,int num);
extern int SSL_read(SSL *ssl, void *buf,int num);
#endif

#if defined(CONFIG_APP_FWD)
//keith_fwd
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

static int shm_id=0, shm_size;
char *shm_memory=NULL; //, *shm_name;

int get_shm_id()
{
	return shm_id;
}

int set_shm_id(int id)
{
	 shm_id=id;
}

int clear_fwupload_shm(int shmid)
{
	//shm_memory = (char *)shmat(shmid, NULL, 0);
	if(shm_memory){
		if (shmdt(shm_memory) == -1) {
			fprintf(stderr, "shmdt failed\n");
		}
	}

	if (shm_id != 0) {
		if(shmctl(shm_id, IPC_RMID, 0) == -1)
			fprintf(stderr, "shmctl(IPC_RMID) failed\n");
	}
	shm_id = 0;
	shm_memory = NULL;
}
//keith_fwd
#endif //#if defined(CONFIG_APP_FWD)



#ifdef HTTP_FILE_SERVER_SUPPORTED
extern void websCheckAction(request *wp);
extern int websDataWrite2File(request *wp, char *socket_data, int data_len);
#endif

/*
 * Name: read_header
 * Description: Reads data from a request socket.  Manages the current
 * status via a state machine.  Changes status from READ_HEADER to
 * READ_BODY or WRITE as necessary.
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: request done, close it down
 *   1: more to do, leave on ready list
 */

int read_header(request * req)
{
	//printf("%s\n",__FUNCTION__);
	int bytes;
	char *check, *buffer;
	unsigned char uc;

	check = req->client_stream + req->parse_pos;
	buffer = req->client_stream;
	bytes = req->client_stream_pos;

	DEBUG(DEBUG_HEADER_READ) {
		if (check < (buffer + bytes)) {
			buffer[bytes] = '\0';
			log_error_time();
			fprintf(stderr, "%s:%d - Parsing headers (\"%s\")\n",
					__FILE__, __LINE__, check);
		}
	}
	while (check < (buffer + bytes)) {
		/* check for illegal characters here
		 * Anything except CR, LF, and US-ASCII - control is legal
		 * We accept tab but don't do anything special with it.
		 */
		uc = *check;
		if (uc != '\r' && uc != '\n' && uc != '\t' &&
				(uc < 32 || uc > 127)) {
			log_error_doc(req);
			DEBUG(DEBUG_BOA) {
				fprintf(stderr, "Illegal character (%d) in stream.\n", (unsigned int) uc);
			}
			send_r_bad_request(req);
			return 0;
		}
		switch (req->status) {
			case READ_HEADER:
				if (uc == '\r') {
					req->status = ONE_CR;
					req->header_end = check;
				} else if (uc == '\n') {
					req->status = ONE_LF;
					req->header_end = check;
				}
				break;

			case ONE_CR:
				if (uc == '\n')
					req->status = ONE_LF;
				else if (uc != '\r')
					req->status = READ_HEADER;
				break;

			case ONE_LF:
				/* if here, we've found the end (for sure) of a header */
				if (uc == '\r') /* could be end o headers */
					req->status = TWO_CR;
				else if (uc == '\n')
					req->status = BODY_READ;
				else
					req->status = READ_HEADER;
				break;

			case TWO_CR:
				if (uc == '\n')
					req->status = BODY_READ;
				else if (uc != '\r')
					req->status = READ_HEADER;
				break;

			default:
				break;
		}

#ifdef VERY_FASCIST_LOGGING
		log_error_time();
		fprintf(stderr, "status, check (unsigned): %d, %d\n", req->status, uc);
#endif

		req->parse_pos++;       /* update parse position */
		check++;

		if (req->status == ONE_LF) {
			*req->header_end = '\0';

			if (req->header_end - req->header_line >= MAX_HEADER_LENGTH) {
				log_error_doc(req);
				DEBUG(DEBUG_BOA) {
					fprintf(stderr, "Header too long at %lu bytes: \"%s\"\n",
							(unsigned long) (req->header_end - req->header_line),
							req->header_line);
				}
				send_r_bad_request(req);
				return 0;
			}

			/* terminate string that begins at req->header_line */

			if (req->logline) {
				if (process_option_line(req) == 0) {
					/* errors already logged */
					return 0;
				}
			} else {
				if (process_logline(req) == 0)
					/* errors already logged */
					return 0;
				if (req->http_version == HTTP09)
					return process_header_end(req);
			}
			/* set header_line to point to beginning of new header */
			req->header_line = check;
		} else if (req->status == BODY_READ) {
#ifdef VERY_FASCIST_LOGGING
			int retval;
			log_error_time();
			fprintf(stderr, "%s:%d -- got to body read.\n",
					__FILE__, __LINE__);
			retval = process_header_end(req);
#else
			int retval = process_header_end(req);
#endif
			/* process_header_end inits non-POST CGIs */

			if (retval && req->method == M_POST) {
				/* for body_{read,write}, set header_line to start of data,
				   and header_end to end of data */
				req->header_line = check;
				req->header_end =
					req->client_stream + req->client_stream_pos;

				req->status = BODY_WRITE;
				/* so write it */
				/* have to write first, or read will be confused
				 * because of the special case where the
				 * filesize is less than we have already read.
				 */

				/*

				   As quoted from RFC1945:

				   A valid Content-Length is required on all HTTP/1.0 POST requests. An
				   HTTP/1.0 server should respond with a 400 (bad request) message if it
				   cannot determine the length of the request message's content.

*/

				if (req->content_length) {
#if defined(ENABLE_LFS)
					off64_t content_length;
#else
					int content_length;
#endif                    

					content_length = boa_atoi(req->content_length);
					/* Is a content-length of 0 legal? */
					if (content_length < 0) {
						log_error_doc(req);
						fprintf(stderr,
								"Invalid Content-Length [%s] on POST!\n",
								req->content_length);
						send_r_bad_request(req);
						return 0;
					}
#ifndef HTTP_FILE_SERVER_SUPPORTED
					if (single_post_limit
							&& content_length > single_post_limit) {
						log_error_doc(req);
						fprintf(stderr,
								"Content-Length [%d] > SinglePostLimit [%d] on POST!\n",
								content_length, single_post_limit);
						send_r_bad_request(req);
						return 0;
					}
#endif
					req->filesize = content_length;
					req->filepos = 0;
					if ((unsigned) (req->header_end - req->header_line) > req->filesize) {
						req->header_end = req->header_line + req->filesize;
					}
#ifdef HTTP_FILE_SERVER_SUPPORTED
					websCheckAction(req);
#endif
				} else {
					log_error_doc(req);
					fprintf(stderr, "Unknown Content-Length POST!\n");
					send_r_bad_request(req);
					return 0;
				}
			}                   /* either process_header_end failed or req->method != POST */
			return retval;      /* 0 - close it done, 1 - keep on ready */
		}                       /* req->status == BODY_READ */
	}//endwhile                           /* done processing available buffer */

#ifdef VERY_FASCIST_LOGGING
	log_error_time();
	fprintf(stderr, "%s:%d - Done processing buffer.  Status: %d\n",
			__FILE__, __LINE__, req->status);
#endif

	if (req->status < BODY_READ) {
		/* only reached if request is split across more than one packet */
		unsigned int buf_bytes_left;

		buf_bytes_left = CLIENT_STREAM_SIZE - req->client_stream_pos;
		if (buf_bytes_left < 1 || buf_bytes_left > CLIENT_STREAM_SIZE) {
			log_error_doc(req);
			DEBUG(DEBUG_BOA) {
				fputs("No space left in client stream buffer, closing\n",
						stderr);
			}
			req->response_status = 400;
			req->status = DEAD;
			return 0;
		}

#ifdef BOA_WITH_OPENSSL
		if(req->ssl == NULL)
#endif
		{
#ifdef BOA_WITH_MBEDTLS
            if(req->mbedtls_client_fd.fd != -1)
            {
				int ret, len;
                /*
                 * 6. Read the HTTP Request
                 */
                mbedtls_printf( "  < Read from client:" );
                fflush( stdout );

                do
                {
                    ret = mbedtls_ssl_read( &(req->mbedtls_ssl_ctx), buffer + req->client_stream_pos, buf_bytes_left );

                    if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
                        continue;

                    if( ret < 0 )
                    {
                        switch( ret )
                        {
                            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                                mbedtls_printf( " connection was closed gracefully\n" );
                                break;

                            case MBEDTLS_ERR_NET_CONN_RESET:
                                mbedtls_printf( " connection was reset by peer\n" );
                                break;

                            default:
                                mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n", -ret );
                                break;
                        }

                        break;
                    }

                    len = ret;
                    //mbedtls_printf( " %d bytes read\n\n%s", len, (char *) buffer + req->client_stream_pos );

                    if( ret >= 0 )
                        break;
                }
                while( 1 );
                bytes = ret;
            }else
#endif
            {
			bytes = read(req->fd, buffer + req->client_stream_pos, buf_bytes_left);
		}
            //printf("<%s:%d>read %d bytes\n%s\n",__FUNCTION__,__LINE__,bytes, buffer + req->client_stream_pos);
        }
#ifdef BOA_WITH_OPENSSL
		else{
//			printf("<%s:%d>SSL_read ", __FUNCTION__, __LINE__);
			bytes = SSL_read(req->ssl, buffer + req->client_stream_pos, buf_bytes_left);
			//printf("bytes=%d\n",bytes);
		}
#endif
		if (bytes < 0) {
			if (errno == EINTR)
				return 1;
			else if (errno == EAGAIN || errno == EWOULDBLOCK) /* request blocked */
				return -1;
			log_error_doc(req);
			perror("header read"); /* don't need to save errno because log_error_doc does */
			req->response_status = 400;
			return 0;
		} else if (bytes == 0) {
#if 0
			printf("<%s:%d>req->kacount=%d, ka_max=%d\n",__FUNCTION__,__LINE__,req->kacount, ka_max);
			if(!req->logline)
				printf("req->logline==NULL\n");
			printf("client_stream_pos=%d\n", req->client_stream_pos);
#endif
			//if (req->kacount < ka_max &&
			if (req->kacount <= ka_max &&
					!req->logline &&
					req->client_stream_pos == 0) {
				/* A keepalive request wherein we've read
				 * nothing.
				 * Ignore.
				 */
				;
			} else {
#ifndef QUIET_DISCONNECT
				log_error_doc(req);
				fputs("client unexpectedly closed connection.\n", stderr);
				fprintf(stderr, "%s -- request \"%s\" (\"%s\"): \n",
						req->remote_ip_addr,
						(req->logline ? req->logline : "(null)"),
						(req->pathname ? req->pathname : "(null)"));
#endif
			}
			req->response_status = 400;
			return 0;
		}

		/* bytes is positive */
		req->client_stream_pos += bytes;

		DEBUG(DEBUG_HEADER_READ) {
			log_error_time();
			req->client_stream[req->client_stream_pos] = '\0';
			fprintf(stderr, "%s:%d -- We read %d bytes: \"%s\"\n",
					__FILE__, __LINE__, bytes,
#ifdef VERY_FASCIST_LOGGING2
					req->client_stream + req->client_stream_pos - bytes
#else
					""
#endif
			       );
		}

		return 1;
	}
	return 1;
}

/*
 * Name: read_body
 * Description: Reads body from a request socket for POST CGI
 *
 * Return values:
 *
 *  -1: request blocked, move to blocked queue
 *   0: request done, close it down
 *   1: more to do, leave on ready list
 *

 As quoted from RFC1945:

 A valid Content-Length is required on all HTTP/1.0 POST requests. An
 HTTP/1.0 server should respond with a 400 (bad request) message if it
 cannot determine the length of the request message's content.

*/

int read_body(request * req)
{
	//printf("%s\n",__FUNCTION__);
#if defined(ENABLE_LFS) 
	off64_t bytes_read;
	off64_t bytes_to_read, bytes_free;
#else
	int bytes_read;
	unsigned int bytes_to_read, bytes_free;
#endif

	bytes_free = BUFFER_SIZE - (req->header_end - req->header_line);
	bytes_to_read = req->filesize - req->filepos;

	if (bytes_to_read > bytes_free)
		bytes_to_read = bytes_free;

	if (bytes_to_read <= 0) {
		req->status = BODY_WRITE; /* go write it */
		return 1;
	}

#ifdef BOA_WITH_OPENSSL
	if(req->ssl == NULL)
#endif
	{
#ifdef BOA_WITH_MBEDTLS
        if(req->mbedtls_client_fd.fd != -1)
        {
        	int ret, len;
            do
            {
                ret = mbedtls_ssl_read( &(req->mbedtls_ssl_ctx), req->header_end, bytes_to_read );

                if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
                    continue;

                if( ret < 0 )
                {
                    switch( ret )
                    {
                        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                            mbedtls_printf( " connection was closed gracefully\n" );
                            break;

                        case MBEDTLS_ERR_NET_CONN_RESET:
                            mbedtls_printf( " connection was reset by peer\n" );
                            break;

                        default:
                            mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n", -ret );
                            break;
                    }

                    break;
                }

                len = ret;
                //mbedtls_printf( " %d bytes read\n\n%s", len, (char *) req->header_end );
                bytes_read = len;

                if( ret >= 0 )
                    break;
            }
            while( 1 );
        }else
#endif
        {
		bytes_read = read(req->fd, req->header_end, bytes_to_read);
	}
        //printf("<%s:%d>read %d bytes\n",__FUNCTION__, __LINE__, bytes_read);
    }
#ifdef BOA_WITH_OPENSSL
	else{
		//printf("<%s:%d>SSL_read\n",__FUNCTION__, __LINE__);
		bytes_read = SSL_read(req->ssl, req->header_end, bytes_to_read);
	}
#endif

	if (bytes_read == -1) {
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return -1;
		} else {
			boa_perror(req, "read body");
			req->response_status = 400;
			return 0;
		}
	} else if (bytes_read == 0) {
		/* this is an error.  premature end of body! */
		log_error_doc(req);
		fprintf(stderr, "%s:%d - Premature end of body!!\n",
				__FILE__, __LINE__);
		send_r_bad_request(req);
		return 0;
	}

	req->status = BODY_WRITE;

#ifdef FASCIST_LOGGING1
	log_error_time();
	fprintf(stderr, "%s:%d - read %d bytes.\n",
			__FILE__, __LINE__, bytes_to_read);
#endif

	req->header_end += bytes_read;

	return 1;
}

/*
 * Name: write_body
 * Description: Writes a chunk of data to a file
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */

int write_body(request * req)
{
#if defined(ENABLE_LFS) 
	off64_t bytes_written;
	off64_t bytes_to_write = req->header_end - req->header_line;
#else

	int bytes_written;
	unsigned int bytes_to_write = req->header_end - req->header_line;
#endif

#if defined(CONFIG_APP_FWD)
	int content_length = boa_atoi(req->content_length);
	if(content_length<0)
		return -1;
	//    printf("%s:%d content_length=%d\n",__FUNCTION__,__LINE__,content_length);
#ifdef CONFIG_RTL_WAPI_SUPPORT  //and cbo --when upload wapi certs, don't kill daemon
	if(content_length>0 && content_length<2000)
		req->daemon_killed=1;		
#endif
#endif


	if (req->filepos + bytes_to_write > req->filesize)
		bytes_to_write = req->filesize - req->filepos;

	if (bytes_to_write == 0) {  /* nothing left in buffer to write */
		req->header_line = req->header_end = req->buffer;
		if (req->filepos >= req->filesize) {
#ifdef SUPPORT_ASP
			extern int init_form(request * req); //davidhsu
#if defined(BOA_CGI_SUPPORT)
			if(req->cgi_type==CGI)
				return init_cgi(req);
			else
#endif			
			return init_form(req);	/* init_cgi should change status approp. */
#else
			return init_cgi(req);
#endif
		}
		/* if here, we can safely assume that there is more to read */
		req->status = BODY_READ;
		return 1;
	}

	// davidhsu -------------------
#ifdef SUPPORT_ASP
	//keith_fwd

#if defined(CONFIG_APP_FWD)
	if (req->upload_data) {

		if (req->daemon_killed == 0) {
#ifndef NO_ACTION
			extern void killDaemon(int wait);
			killDaemon(1);
#endif
			req->daemon_killed = 1;
		}


		if(shm_id == 0)
		{
			/* Generate a System V IPC key */ 
			key_t key;
			key = ftok("/bin/fwd", 0xE04);

			/* Allocate a shared memory segment */
			shm_id = shmget(key, content_length, IPC_CREAT | IPC_EXCL | 0666);	

			if (shm_id == -1) {
				boa_perror(req, "upload image too big!");
				return -2;
			}
			/* Attach the shared memory segment */

			shm_memory = (char *)shmat(shm_id, NULL, 0);
			if(req->upload_data !=NULL )
			{
				free(req->upload_data);
			}
			req->upload_data = shm_memory;
		}


		memcpy(&req->upload_data[req->upload_len], req->header_line, bytes_to_write);
		req->upload_len += bytes_to_write;
		req->header_line += bytes_to_write;
		req->filepos += bytes_to_write;

		return 1;
	}



#else
	if (req->upload_data) {
		if (req->upload_len+bytes_to_write > MAX_UPLOAD_SIZE) {		
			req->upload_data = realloc(req->upload_data, req->upload_len+bytes_to_write);
			if (req->upload_data == NULL) {
				boa_perror(req, "upload image too big!");
				return -2;
			}
		}

		memcpy(&req->upload_data[req->upload_len], req->header_line, bytes_to_write);
		req->upload_len += bytes_to_write;
		req->header_line += bytes_to_write;
		req->filepos += bytes_to_write;
		if (req->upload_len > (MAX_UPLOAD_SIZE/2) && !req->daemon_killed) {
#ifndef NO_ACTION
			extern void killDaemon(int wait);
			killDaemon(1);
#endif
			req->daemon_killed = 1;
		}
		return 1;
	}
#endif ////keith_fwd	
#endif	
	//-----------------------------
	// davidhsu    
#ifndef NEW_POST
	bytes_written = write(req->post_data_fd,
			req->header_line, bytes_to_write);
#else
#ifdef HTTP_FILE_SERVER_SUPPORTED
	if (req->FileUploadAct==1) {
		int file_write_st=0;
		//websReSettimeout(req);
		file_write_st=websDataWrite2File(req, req->header_line, bytes_to_write);
		bytes_written = bytes_to_write;
	}
	else
#endif
#if defined(BOA_CGI_SUPPORT)
	if(req->cgi_type==CGI){
		bytes_written = write(req->post_data_fd,
							  req->header_line, bytes_to_write);
	}else
#endif
	{
		if ((req->post_data_len + bytes_to_write)  > BUFFER_SIZE) {
			printf("error, post_data_len > BUFFER_SIZE!\n");
			return -1;		
		}	
		memcpy(&req->post_data[req->post_data_idx], req->header_line, bytes_to_write);
		req->post_data_len += bytes_to_write;
		req->post_data_idx += bytes_to_write;	
		bytes_written = bytes_to_write;
	}
#endif	
	//-------------------------------	

	if (bytes_written < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return -1;          /* request blocked at the pipe level, but keep going */
		else if (errno == EINTR)
			return 1;
		else if (errno == ENOSPC) {
			/* 20010520 - Alfred Fluckiger */
			/* No test was originally done in this case, which might  */
			/* lead to a "no space left on device" error.             */
			boa_perror(req, "write body"); /* OK to disable if your logs get too big */
			return 0;
		} else {
			boa_perror(req, "write body"); /* OK to disable if your logs get too big */
			return 0;
		}
	}
	DEBUG(DEBUG_HEADER_READ) {
		log_error_time();
		fprintf(stderr, "%s:%d - wrote %d bytes of CGI body. %ld of %ld\n",
				__FILE__, __LINE__,
				bytes_written, req->filepos, req->filesize);
	}

	req->filepos += bytes_written;
	req->header_line += bytes_written;

	DEBUG(DEBUG_CGI_INPUT) {
		log_error_time();
		{
			char c = req->header_line[bytes_written];

			req->header_line[bytes_written] = '\0';
			fprintf(stderr,
					"%s:%d - wrote %d bytes (%s). %lu of %lu\n",
					__FILE__, __LINE__, bytes_written,
					req->header_line, req->filepos, req->filesize);
			req->header_line[bytes_written] = c;
		}
	}

	return 1;                   /* more to do */
}
