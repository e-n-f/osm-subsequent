#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

// boilerplate from
// http://marcomaggi.github.io/docs/expat.html#overview-intro
// Copyright 1999, Clark Cooper

#define BUFFSIZE        8192

long long seq = 0;
int within = 0;

char **users = NULL;
int nusers;

void quote(const char *s) {
	for (; *s; s++) {
		if (*s == '"') {
			printf("&quot;");
		} else if (*s == '&') {
			printf("&amp;");
		} else {
			putchar(*s);
		}
	}
}

static void XMLCALL start(void *data, const char *element, const char **attribute) {
	static long long printid = -1;

	if (strcmp(element, "node") == 0 || strcmp(element, "way") == 0) {
		long long id = -1;
		int want = 0;

		int i;
		for (i = 0; attribute[i] != NULL; i += 2) {
			if (strcmp(attribute[i], "id") == 0) {
				id = atoll(attribute[i + 1]);
			} else if (strcmp(attribute[i], "user") == 0) {
				int j;

				for (j = 0; j < nusers; j++) {
					if (strcmp(attribute[i + 1], users[j]) == 0) {
						want = 1;
						break;
					}
				}
			}
		}

		if (want) {
			printid = id;
		} else if (id != printid) {
			printid = -1;
		}

		if (printid == id) {
			printf("	<%s", element);

			for (i = 0; attribute[i] != NULL; i += 2) {
				printf(" ");
				quote(attribute[i]);
				printf("=\"");
				quote(attribute[i + 1]);
				printf("\"");
			}

			printf(">\n");
			within = 1;
		}

		if (seq++ % 100000 == 0) {
			fprintf(stderr, "node %lld  \r", id);
		}
	} else if (strcmp(element, "changeset") == 0) {
		if (seq++ % 100000 == 0) {
			unsigned int id = 0;
			int i;
			for (i = 0; attribute[i] != NULL; i += 2) {
				if (strcmp(attribute[i], "id") == 0) {
					id = atoi(attribute[i + 1]);
				}
			}

			fprintf(stderr, "changeset %u  \r", id);
		}
	} else if (strcmp(element, "nd") == 0 || strcmp(element, "tag") == 0) {
		if (within) {
			printf("		<%s", element);

			int i;
			for (i = 0; attribute[i] != NULL; i += 2) {
				printf(" ");
				quote(attribute[i]);
				printf("=\"");
				quote(attribute[i + 1]);
				printf("\"");
			}

			printf("/>\n");
		}
	}
}

static void XMLCALL end(void *data, const char *el) {
	if (within) {
		if (strcmp(el, "way") == 0 || strcmp(el, "node") == 0) {
			printf("	</%s>\n", el);
			within = 0;
		}
	}
}

int main(int argc, char *argv[]) {
	users = argv + 1;
	nusers = argc - 1;

	if (nusers <= 0) {
		fprintf(stderr, "Usage: %s user ...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	XML_Parser p = XML_ParserCreate(NULL);
	if (p == NULL) {
		fprintf(stderr, "Couldn't allocate memory for parser\n");
		exit(EXIT_FAILURE);
	}

	XML_SetElementHandler(p, start, end);

	printf("<?xml version='1.0' encoding='UTF-8'?>\n");
	printf("<osm version=\"0.6\" generator=\"osmconvert 0.7P\">\n");

	int done = 0;
	while (!done) {
		int len;
		char Buff[BUFFSIZE];

		len = fread(Buff, 1, BUFFSIZE, stdin);
		if (ferror(stdin)) {
       			fprintf(stderr, "Read error\n");
			exit(EXIT_FAILURE);
		}
		done = feof(stdin);

		if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR) {
			fprintf(stderr, "Parse error at line %lld:\n%s\n", (long long) XML_GetCurrentLineNumber(p), XML_ErrorString(XML_GetErrorCode(p)));
			exit(EXIT_FAILURE);
		}
	}

	XML_ParserFree(p);

	printf("</osm>\n");
	return 0;
}
