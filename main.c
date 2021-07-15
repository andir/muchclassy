#include <unistd.h>
#include <stdio.h>
#include <notmuch.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

static int classify_mail(notmuch_message_t* message, const char* filename) {

	int changed = 0;
	const char* header = get_header_from_file(filename, "List-Id");
	// const char* header = notmuch_message_get_header(message, "List-Id");

	if (header != NULL && header[0] != '\0') {
		char* identifier = get_list_identifier(header);
		free((char*)header);

		if (identifier != NULL) {
			char *list_tag = list_identifer_to_tag(identifier);
			const size_t list_tag_len = strlen(list_tag);
			printf("list: %s %s\n", identifier, list_tag);


			// check if the tag was already applied, if not appliy it
			int found = 0;
			for (notmuch_tags_t * tags = notmuch_message_get_tags(message);
			     notmuch_tags_valid(tags);
			     notmuch_tags_move_to_next(tags)) {
				const char * tag = notmuch_tags_get(tags);
				if (strncmp(list_tag, tag,list_tag_len) == 0) {
					found = 1;
					break;
				}
			}

			if (!found) {
				const char* message_id = notmuch_message_get_message_id(message);
				notmuch_message_add_tag(message, list_tag);
				printf("adding tag %s to message %s\n", list_tag, message_id);
				changed = 1;
			}


			free(list_tag);
			free(identifier);
		}
	}

	return changed;
}


int main(int argc, char* argv[]) {
	int remove_new_tag = 0, opt;
	char* search_term = NULL;
	notmuch_database_t* database;

	while ((opt = getopt(argc, argv, "n:")) != -1) {
		switch (opt) {
		case 'n':
			remove_new_tag = 1;
			break;
		default:
			fprintf(stderr, "usage:\n\t%s [-n] <search-term>\n", argc > 0 ? argv[0] : "<bin>");
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		search_term = "tag:new";
	} else {
		search_term = argv[optind];
	}


	notmuch_status_t rc = notmuch_database_open("/home/andi/Maildir", NOTMUCH_DATABASE_MODE_READ_WRITE, &database);

	if (rc != NOTMUCH_STATUS_SUCCESS) {
		return 1;
	}

	fprintf(stderr, "Searching for messages matching the query: %s\n", search_term);

	notmuch_query_t * query = notmuch_query_create(database, search_term);
	notmuch_messages_t* messages;
	notmuch_message_t* message;

	for (rc = notmuch_query_search_messages(query, &messages);
	     rc == NOTMUCH_STATUS_SUCCESS &&
	     notmuch_messages_valid(messages);
	     notmuch_messages_move_to_next(messages))
	{
		message = notmuch_messages_get(messages);

		if (message == NULL)
			break; // OOM

		const char* message_id = notmuch_message_get_message_id(message);
		fprintf(stderr, "processing: %s\n", message_id);

		if (notmuch_message_freeze(message) != NOTMUCH_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to freeze message. Aborting\n");
			notmuch_message_destroy(message);
			break;
		}

		int changed = 0;
		notmuch_filenames_t* filenames = notmuch_message_get_filenames(message);
		for (notmuch_filenames_t* filenames = notmuch_message_get_filenames(message);
			filenames != NULL &&
			notmuch_filenames_valid(filenames);
			notmuch_filenames_move_to_next(filenames)) {

			const char* filename = notmuch_filenames_get(filenames);
			fprintf(stderr, "filename: %s\n", filename);
			if (filename == NULL) continue;

			changed |= classify_mail(message, filename);
		}

		notmuch_filenames_destroy(filenames);

		if (remove_new_tag) {
			changed |= 1;
			notmuch_message_remove_tag(message, "new"); // ignore returned value
		}

		if (changed) {

			int res = notmuch_message_thaw(message);
			if (res != NOTMUCH_STATUS_SUCCESS) {
				fprintf(stderr, "Failed to thaw changes to message. Aborting\n");
				notmuch_message_destroy(message);
				break;
			}
		}

		notmuch_message_destroy(message);
	}

	notmuch_query_destroy(query);

	notmuch_database_close(database);

	return 0;
}
