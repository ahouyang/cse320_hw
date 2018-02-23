/*
 * Example of the use of the HTTP package for "Internet Bumbler"
 * E. Stark, 11/18/97 for CSE 230
 *
 * This program takes a single argument, which is interpreted as the
 * URL of a document to be retrieved from the Web.  It uses the HTTP
 * package to parse the URL, make an HTTP connection to the remote
 * server, retrieve the document, and display the result code and string,
 * the response headers, and the data portion of the document.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "http.h"
#include "url.h"
#include "snarf.h"

int
main(int argc, char *argv[])
{
  URL *up;
  HTTP *http;
  IPADDR *addr;
  int port, c, code;
  char *status, *method;
  parse_args(argc, argv);
  if((up = url_parse(url_to_snarf)) == NULL) {
    fprintf(stderr, "Illegal URL: '%s'\n", argv[1]);
    //url_free(up);
    exit(1);
  }
  method = url_method(up);
  addr = url_address(up);
  port = url_port(up);
  if(method == NULL || strcasecmp(method, "http")) {
    fprintf(stderr, "Only HTTP access method is supported\n");
    url_free(up);
    exit(1);
  }
  if((http = http_open(addr, port)) == NULL) {
    fprintf(stderr, "Unable to contact host '%s', port %d\n",
	    url_hostname(up) != NULL ? url_hostname(up) : "(NULL)", port);
    //url_free(up);
    //http_close(http);
    exit(1);
  }
  http_request(http, up);
  /*
   * Additional RFC822-style headers can be sent at this point,
   * if desired, by outputting to url_file(up).  For example:
   *
   *     fprintf(url_file(up), "If-modified-since: 10 Jul 1997\r\n");
   *
   * would activate "Conditional GET" on most HTTP servers.
   */
  http_response(http);
  /*
   * At this point, response status and headers are available for querying.
   *
   * Some of the possible HTTP response status codes are as follows:
   *
   *	200	Success -- document follows
   *	302	Document moved
   *	400	Request error
   *	401	Authentication required
   *	403	Access denied
   *	404	Document not found
   *	500	Server error
   *
   * You probably want to examine the "Content-type" header.
   * Two possibilities are:
   *    text/html	The body of the document is HTML code
   *	text/plain	The body of the document is plain ASCII text
   */
  status = http_status(http, &code);
  //create output file
  FILE* outputFile;
  if(output_file != NULL)
    outputFile = fopen(output_file, "w");

  //lookup keywords in header
  for(int i = 0; i < numKeywords; i++){
    char *result = http_headers_lookup(http, keywords[i]);
    char *key = http_key_lookup(http, keywords[i]);
    if(result != NULL){
      fprintf(stderr, "%s: %s\n",key, result);
    }
  }
  //if(numKeywords > 0){
    //fprintf(stderr, "\n");
  //}

#ifdef DEBUG
  debug("%s", status);
#else
  (void) status;
#endif
  /*
   * At this point, we can retrieve the body of the document,
   * character by character, using http_getc()
   */
  while((c = http_getc(http)) != EOF){
    if(output_file == NULL){
    putchar(c);
    }
    else{
      putc(c, outputFile);
    }
  }

  if(strcmp(status,"200") == 0){
    http_close(http);
    url_free(up);
    exit(0);
  }
  else{
    int intStatus = atoi(status);
    http_close(http);
    url_free(up);
    exit(intStatus);
  }
  //return(0);
}
