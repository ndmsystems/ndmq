/**************************************************************************
 * Copyright (c) 2015 NDM Systems, Inc. http://www.ndmsystems.com/
 * This software is freely distributable, see COPYING for details.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************/

/* vi: set tabstop=4: */

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

/* libndm headers */
#include <ndm/core.h>
#include <ndm/log.h>
#include <ndm/xml.h>

#include "version.h"

#define NDMQ_CACHE_TTL_MS_				1000
#define NDMQ_PARSE_MAX_SIZE_			4096
#define NDMQ_AGENT_DEFAULT_				"ndmq/ci"
#define NDMQ_AGENT_MAX_SIZE_			64
#define NDMQ_XPATH_DEFAULT_				"/"
#define NDMQ_XPATH_MAX_SIZE_			256
#define NDMQ_XCOPY_STATIC_BUFFER_		1024
#define NDMQ_XCOPY_DYNAMIC_BLOCK_		1024

enum ndmq_print_xml_t
{
	NDMQ_PRINT_XML_NO_					= 0,
	NDMQ_PRINT_XML_YES_					= 1
};

static int ndmq_show_version_(const char* const ident)
{
	fprintf(stderr, "NDM Query (%s), version %s\n"
			"Copyright (c) 2015 NDM Systems, Inc.\n",
			ident, NDMQ_VERSION);

	return EXIT_SUCCESS;
}

static int ndmq_show_usage_(const char* const ident)
{
	ndmq_show_version_(ident);

	fprintf(stderr, "\nUsage: \n");
	fprintf(stderr, "\t%s [OPTION]... -p REQUEST\n", ident);
	fprintf(stderr, "\t%s [ -h | -v ]\n", ident);
	fprintf(stderr, "\nOptions:\n"
	"\t-p REQUEST    <parse> request (maximum %d chars)\n"
	"\t-A AGENT      agent name, defaults to \"%s\" (maximum %d chars)\n"
	"\t-P XPATH      response path, defaults to \"%s\" (maximum %d chars)\n"
	"\t              note: path syntax is limited to element names only,\n"
	"\t                    for example, use -P \"interface/id\" to show\n"
	"\t                    values of <id> children of <interface> elements\n"
	"\t-x            print nested XML nodes\n"
	"\n"
	"\t-h            display this help and exit\n"
	"\t-v            display version information and exit\n\n",
			NDMQ_PARSE_MAX_SIZE_,
			NDMQ_AGENT_DEFAULT_, NDMQ_AGENT_MAX_SIZE_,
			NDMQ_XPATH_DEFAULT_, NDMQ_XPATH_MAX_SIZE_);

	return EXIT_SUCCESS;
}

static int ndmq_print_err_(const char* const format, ...)
{
	va_list ap;

	fprintf(stderr, "Error: ");
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");

	return EXIT_FAILURE;
}

/**************************************************************************
 * Response printing
 **************************************************************************/

static void ndmq_print_indent__(
		FILE* out,
		unsigned long indent)
{
	unsigned long i = 0;

	for (i = 0; i < indent; ++i) {
		fprintf(out, "%4s", "");
	}
}

static void ndmq_print_xml__(
		FILE* out,
		const struct ndm_xml_node_t* root,
		unsigned long indent)
{
	const enum ndm_xml_node_type_t type = ndm_xml_node_type(root);
	const char* value = ndm_xml_node_value(root);

	ndmq_print_indent__(out, indent);

	if (type == NDM_XML_NODE_TYPE_DATA) {
		fprintf(out, "%s\n", value);
	} else
	if (type == NDM_XML_NODE_TYPE_ELEMENT) {
		const struct ndm_xml_node_t* node =
			ndm_xml_node_first_child(root, NULL);
		const struct ndm_xml_attr_t* attr =
			ndm_xml_node_first_attr(root, NULL);
		const char* name = ndm_xml_node_name(root);

		fprintf(out, "<%s", name);

		while (attr != NULL) {
			fprintf(out, " %s=\"%s\"",
					ndm_xml_attr_name(attr),
					ndm_xml_attr_value(attr));
			attr = ndm_xml_attr_next(attr, NULL);
		}

		if (node != NULL) {
			fprintf(out, ">\n");
			while (node != NULL) {
				ndmq_print_xml__(out, node, indent + 1);
				node = ndm_xml_node_next_sibling(node, NULL);
			}
			ndmq_print_indent__(out, indent);
			fprintf(out, "</%s>\n", name);
		} else
		if (value[0] != '\0') {
			fprintf(out, ">%s</%s>\n", value, name);
		} else {
			fprintf(out, "/>\n");
		}
	}
}

static int ndmq_print_xml_(
		FILE* out,
		const struct ndm_xml_node_t* root,
		const char* xpath,
		const enum ndmq_print_xml_t print_xml)
{
	const char* p = xpath;

	while (*p == '/') {
		++p;
	}

	if (*p == '\0') {
		const char* value = ndm_xml_node_value(root);

		if (value[0] != '\0') {
			fprintf(out, "%s\n", value);
		} else
		if( print_xml ) {
			ndmq_print_xml__(out, root, 0);
		}

		return EXIT_SUCCESS;
	}

	char name[NDMQ_XPATH_MAX_SIZE_ + 1];
	struct ndm_xml_node_t* n = NULL;
	const char* q = p;

	while (q < p + (sizeof(name) - 1)) {
		if( *q == '/' || *q == '\0') {
			break;
		}

		name[q - p] = *q;
		++q;
	}

	name[q - p] = '\0';
	n = ndm_xml_node_first_child(root, name);

	while (n != NULL) {
		ndmq_print_xml_(out, n, q, print_xml);
		n = ndm_xml_node_next_sibling(n, name);
	}

	return EXIT_SUCCESS;
}

/**************************************************************************
 * Resource cleanup
 *
 * The below functions should never be called more than once.
 **************************************************************************/

void ndmq_atexit_core_close_(struct ndm_core_t* core);
void ndmq_atexit_core_response_free_(struct ndm_core_response_t* resp);

static void ndmq_core_close_()
{
	ndmq_atexit_core_close_(NULL);
}

static void ndmq_core_response_free_()
{
	ndmq_atexit_core_response_free_(NULL);
}

void ndmq_atexit_core_close_(struct ndm_core_t* core)
{
	static struct ndm_core_t* core_ = NULL;

	if (core != NULL) {
		assert( core_ == NULL );
		core_ = core;
		atexit(ndmq_core_close_);
	} else {
		ndm_core_close(&core_);
	}
}

void ndmq_atexit_core_response_free_(struct ndm_core_response_t* resp)
{
	static struct ndm_core_response_t* resp_ = NULL;

	if (resp != NULL) {
		assert( resp_ == NULL );
		resp_ = resp;
		atexit(ndmq_core_response_free_);
	} else {
		ndm_core_response_free(&resp_);
	}
}

/**************************************************************************
 * Main
 **************************************************************************/

int main(int argc, char *argv[])
{
	const char *const ident = ndm_log_get_ident(argv);

	if (argc == 1) {

		exit(ndmq_show_usage_(ident));
	}

	char parse[NDMQ_PARSE_MAX_SIZE_ + 1];
	char agent[NDMQ_AGENT_MAX_SIZE_ + 1];
	char xpath[NDMQ_XPATH_MAX_SIZE_ + 1];
	enum ndm_core_request_type_t request_type = NDM_CORE_REQUEST_PARSE;
	enum ndmq_print_xml_t print_xml = NDMQ_PRINT_XML_NO_;
	char c = 0;

	snprintf(parse, sizeof(parse), "%s", "");
	snprintf(agent, sizeof(agent), "%s", NDMQ_AGENT_DEFAULT_);
	snprintf(xpath, sizeof(xpath), "%s", NDMQ_XPATH_DEFAULT_);
	opterr = 0;

	while ((c = getopt(argc, argv, ":A:P:p:xhv")) != -1) {
		switch (c) {
			case 'h':
				exit(ndmq_show_usage_(ident));

			case 'v':
				exit(ndmq_show_version_(ident));

			case 'p':
				request_type = NDM_CORE_REQUEST_PARSE;

				if( snprintf(parse, sizeof(parse), "%s", optarg) >=
						sizeof(parse) )
				{
					exit(ndmq_print_err_(
								"parse request is longer than %d chars",
								sizeof(parse) - 1));
				}

				break;

			case 'A':
				if( snprintf(agent, sizeof(agent), "%s", optarg) >=
						sizeof(agent) )
				{
					exit(ndmq_print_err_(
								"agent name is longer than %d chars",
								sizeof(agent) - 1));
				}

				break;

			case 'P':
				if( snprintf(xpath, sizeof(xpath), "%s", optarg) >=
						sizeof(xpath) )
				{
					exit(ndmq_print_err_(
								"xpath is longer than %d chars",
								sizeof(xpath) - 1));
				}

				break;

			case 'x':
				print_xml = NDMQ_PRINT_XML_YES_;

				break;

			case ':':
				exit(ndmq_print_err_(
							"missing argument of \"%c\"", (char) optopt));

			default:
				exit(ndmq_print_err_(
							"unknown option \"%c\"", (char) optopt));

		}
	}

	struct ndm_core_t *core = NULL;

	if ((core = ndm_core_open(agent,
			NDMQ_CACHE_TTL_MS_, NDM_CORE_DEFAULT_CACHE_MAX_SIZE)) == NULL)
	{
		exit(ndmq_print_err_(
					"ndm core connection failed: %s", strerror(errno)));
	} else
	{
		ndmq_atexit_core_close_(core);
	}

	struct ndm_core_response_t *resp = NULL;

	if ((resp = ndm_core_request(core,
			request_type, NDM_CORE_MODE_CACHE, NULL, parse)) == NULL)
	{
		exit(ndmq_print_err_("ndm request failed: %s", strerror(errno)));

	} else
	{
		ndmq_atexit_core_response_free_(resp);
	}

	const struct ndm_xml_node_t* root = ndm_core_response_root(resp);

	if (root == NULL) {
		exit(ndmq_print_err_("null ndm response"));
	}

	if (!ndm_core_response_is_ok(resp)) {
		if (print_xml) {
			ndmq_print_xml_(stderr, root, "", print_xml);
		} else {
			ndmq_print_err_("%s: %s",
					ndm_core_last_message_ident(core),
					ndm_core_last_message_string(core));
		}

		exit(ndm_core_last_message_code(core));
	}

	exit(ndmq_print_xml_(stdout, root, xpath, print_xml));
}
