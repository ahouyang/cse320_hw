#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "snarf.h"

int opterr;
int optopt;
int optind;
char *optarg;

char *url_to_snarf;
char *output_file;

void
parse_args(int argc, char *argv[])
{
  output_file = NULL;
  numKeywords = 0;

  int i;
  char option;

  for (i = 0; optind < argc; i++) {
    debug("%d opterr: %d", i, opterr);
    debug("%d optind: %d", i, optind);
    debug("%d optopt: %d", i, optopt);
    debug("%d argv[optind]: %s", i, argv[optind]);
    if ((option = getopt(argc, argv, "+q:o")) != -1) {
      switch (option) {
        case 'h': {
          USAGE(argv[0]);
          exit(0);
        }
        case 'q': {
          info("Query header: %s", optarg);
          keywords[numKeywords++] = argv[optind - 1];

          //printf("%s\n", argv[i]);
          break;
        }
        case 'o': {
          info("Output file: %s", optarg);
          //printf("%s\n", argv[optind]);
	        output_file = argv[optind];
          if(optind >= argc){
            exit(-1);
          }
          //printf("%s\n", output_file);
          break;
        }
        case '?': {
          if (optopt != 'h')
            fprintf(stderr, KRED "-%c is not a supported argument\n" KNRM,
                    optopt);

          USAGE(argv[0]);
          exit(0);
          break;
        }
        default: {
          break;
        }
      }
    } else if(optind >= argc){
      exit(-1);
    }
    else if(argv[optind] != NULL) {
	info("URL to snarf: %s", argv[optind]);
	url_to_snarf = argv[optind];
	optind++;
    }
  }
}
