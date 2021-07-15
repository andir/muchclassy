#include <stdio.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <gmime/gmime.h>

#include <glib.h> /* GHashTable */

char* get_list_identifier(const char* value) {
	static int init = 0;
	static regex_t regex;

	if (value == NULL) return NULL;

	if (!init) {
		init = 1;
		int rc = regcomp(&regex, "<([^>]+)>", REG_EXTENDED);

		if (rc) {
			fprintf(stderr, "failed to compile regex\n");
			exit(1);
		}
	}

	// needs space for two matches, 1st is the entire string that matched,
	// 2nd is the actual list-id I am interested in
	regmatch_t matches[2] = { 0 };

	int rc = regexec(&regex, value, 2, (regmatch_t*)&matches, 0);
	if (rc == 0) {

		const size_t start_offset = matches[1].rm_so;
		const size_t end_offset = matches[1].rm_eo;

		if (start_offset == 0 || end_offset == 0) {
			fprintf(stderr, "match has zero length: %s\n", value);
			return NULL;
		}

		const size_t len = end_offset - start_offset;
		if (len <= 0) {
			fprintf(stderr, "size <= 0: %lu %lu %lu\n", len, start_offset, end_offset);
			return NULL;
		}

		char* match = calloc(len + 1, sizeof (char));
		if (match == NULL) {
			return NULL;
		}

		strncpy(match, value + start_offset, len);
		return match;
	} else {
		fprintf(stderr, "rc: %i\n", rc);
	}

	return NULL;
}

char* list_identifer_to_tag(const char* identifier) {
	if (identifier == NULL)
		return NULL;

	const size_t len = strlen(identifier);
	if (len <= 0)
		return NULL;

	const char* end = identifier + len;
	char *saveptr = NULL;

	// reverse the element order in the string
	static const char *prefix = "list::";
	char *scratch = calloc(len + 1 + strlen(prefix), sizeof (char));

	char *p = strtok_r((char*)identifier, ".", &saveptr);
	size_t offset = len + strlen(prefix);
	while (p != NULL) {
		if (offset <= 0)
			break;

		if (p >= end)
			break;

		char *next = strtok_r(NULL, ".", &saveptr);
		const size_t tok_len = (next != NULL ? next - p -1 : strlen(p));

		offset -= tok_len;

		if (tok_len == 0)
			break;

		memcpy(scratch + offset, p, tok_len);

		if (offset > strlen(prefix) + 1) {
			scratch[offset-1] = '.';
			offset -= 1;
		}

		p = next;
	}
	memcpy(scratch, prefix, strlen(prefix));

	return scratch;
}


enum parse_file_result {
	SUCCESS,
	ERROR
};

char*
get_header_from_file (const char *filename, const char *header)
{
    char *out = NULL;
    GMimeParser *parser;
    GMimeStream *stream;
    GMimeMessage *message;
    GMimeHeaderList *headers;
    GError *e = NULL;

    static int initialized = 0;

    if (!filename)
	return NULL;

    if (! initialized) {
	g_mime_init ();
	initialized = 1;
    }

    stream = g_mime_stream_file_open(filename, "r", &e);
    if (stream == NULL || e != NULL) {
	if (e == NULL)
	    fprintf(stderr, "failed to open stream\n");
	else
       	    fprintf(stderr, "failed to open stream: e=%s\n", e->message);

	return NULL;
    }

    parser = g_mime_parser_new_with_stream (stream);
    if (parser == NULL)
        return NULL;

    g_mime_parser_set_format (parser, GMIME_FORMAT_MESSAGE);

    message = g_mime_parser_construct_message (parser, NULL);
    if (! message) {
	goto DONE;
    }

    headers = g_mime_object_get_header_list(GMIME_OBJECT(message));
    if (headers == NULL) {
	goto DONE;
    }

    fprintf(stderr, "reading headers for %s\n", filename);
    for (int i = 0; i < g_mime_header_list_get_count (headers); i++) {
	GMimeHeader *g_header = g_mime_header_list_get_header_at(headers, i);
	if (strcasecmp(g_mime_header_get_name(g_header), header) == 0) {
	    const char *value = g_mime_header_get_value (g_header);
	    out = g_mime_utils_header_decode_text(NULL, value);
	    fprintf(stderr, "header value: %s\n", out);
	    goto DONE;
	}
    }


  DONE:
    if (stream) {
	g_mime_stream_close (stream);
	g_object_unref (stream);
    }

    if (parser)
        g_object_unref (parser);

    if (headers)
	g_object_unref (headers);

    return out;
}
