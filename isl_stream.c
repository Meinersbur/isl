/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <isl_ctx.h>
#include "isl_stream.h"

struct isl_keyword {
	char			*name;
	enum isl_token_type	type;
};

static int same_name(const void *entry, const void *val)
{
	const struct isl_keyword *keyword = (const struct isl_keyword *)entry;

	return !strcmp(keyword->name, val);
}

enum isl_token_type isl_stream_register_keyword(struct isl_stream *s,
	const char *name)
{
	struct isl_hash_table_entry *entry;
	struct isl_keyword *keyword;
	uint32_t name_hash;

	if (!s->keywords) {
		s->keywords = isl_hash_table_alloc(s->ctx, 10);
		if (!s->keywords)
			return ISL_TOKEN_ERROR;
		s->next_type = ISL_TOKEN_LAST;
	}

	name_hash = isl_hash_string(isl_hash_init(), name);

	entry = isl_hash_table_find(s->ctx, s->keywords, name_hash,
					same_name, name, 1);
	if (!entry)
		return ISL_TOKEN_ERROR;
	if (entry->data) {
		keyword = entry->data;
		return keyword->type;
	}

	keyword = isl_calloc_type(s->ctx, struct isl_keyword);
	if (!keyword)
		return ISL_TOKEN_ERROR;
	keyword->type = s->next_type++;
	keyword->name = strdup(name);
	if (!keyword->name) {
		free(keyword);
		return ISL_TOKEN_ERROR;
	}
	entry->data = keyword;

	return keyword->type;
}

static struct isl_token *isl_token_new(struct isl_ctx *ctx,
	int line, int col, unsigned on_new_line)
{
	struct isl_token *tok = isl_alloc_type(ctx, struct isl_token);
	if (!tok)
		return NULL;
	tok->line = line;
	tok->col = col;
	tok->on_new_line = on_new_line;
	return tok;
}

void isl_token_free(struct isl_token *tok)
{
	if (!tok)
		return;
	if (tok->type == ISL_TOKEN_VALUE)
		isl_int_clear(tok->u.v);
	else if (tok->type == ISL_TOKEN_IDENT)
		free(tok->u.s);
	free(tok);
}

void isl_stream_error(struct isl_stream *s, struct isl_token *tok, char *msg)
{
	int line = tok ? tok->line : s->line;
	int col = tok ? tok->col : s->col;
	fprintf(stderr, "syntax error (%d, %d): %s\n", line, col, msg);
	if (tok) {
		if (tok->type < 256)
			fprintf(stderr, "got '%c'\n", tok->type);
		else
			fprintf(stderr, "got token type %d\n", tok->type);
	}
}

static struct isl_stream* isl_stream_new(struct isl_ctx *ctx)
{
	int i;
	struct isl_stream *s = isl_alloc_type(ctx, struct isl_stream);
	if (!s)
		return NULL;
	s->ctx = ctx;
	isl_ctx_ref(s->ctx);
	s->size = 256;
	s->file = NULL;
	s->str = NULL;
	s->buffer = isl_alloc_array(ctx, char, s->size);
	if (!s->buffer)
		goto error;
	s->len = 0;
	s->line = 1;
	s->col = 0;
	s->eof = 0;
	s->c = -1;
	for (i = 0; i < 5; ++i)
		s->tokens[i] = NULL;
	s->n_token = 0;
	s->keywords = NULL;
	return s;
error:
	isl_stream_free(s);
	return NULL;
}

struct isl_stream* isl_stream_new_file(struct isl_ctx *ctx, FILE *file)
{
	struct isl_stream *s = isl_stream_new(ctx);
	if (!s)
		return NULL;
	s->file = file;
	return s;
}

struct isl_stream* isl_stream_new_str(struct isl_ctx *ctx, const char *str)
{
    struct isl_stream *s = isl_stream_new(ctx);
    s->str = str;
    return s;
}

static int isl_stream_getc(struct isl_stream *s)
{
	int c;
	if (s->eof)
		return -1;
	if (s->file)
		c = fgetc(s->file);
	else {
		c = *s->str++;
		if (c == '\0')
			c = -1;
	}
	if (c == -1)
		s->eof = 1;
	if (!s->eof) {
		if (s->c == '\n') {
			s->line++;
			s->col = 0;
		} else
			s->col++;
	}
	s->c = c;
	return c;
}

static void isl_stream_ungetc(struct isl_stream *s, int c)
{
	if (s->file)
		ungetc(c, s->file);
	else
		--s->str;
	s->c = -1;
}

static int isl_stream_push_char(struct isl_stream *s, int c)
{
	if (s->len >= s->size) {
		s->size = (3*s->size)/2;
		s->buffer = isl_realloc_array(ctx, s->buffer, char, s->size);
		if (!s->buffer)
			return -1;
	}
	s->buffer[s->len++] = c;
	return 0;
}

void isl_stream_push_token(struct isl_stream *s, struct isl_token *tok)
{
	isl_assert(s->ctx, s->n_token < 5, return);
	s->tokens[s->n_token++] = tok;
}

static enum isl_token_type check_keywords(struct isl_stream *s)
{
	struct isl_hash_table_entry *entry;
	struct isl_keyword *keyword;
	uint32_t name_hash;

	if (!strcasecmp(s->buffer, "exists"))
		return ISL_TOKEN_EXISTS;
	if (!strcasecmp(s->buffer, "and"))
		return ISL_TOKEN_AND;
	if (!strcasecmp(s->buffer, "or"))
		return ISL_TOKEN_OR;
	if (!strcasecmp(s->buffer, "infty"))
		return ISL_TOKEN_INFTY;
	if (!strcasecmp(s->buffer, "infinity"))
		return ISL_TOKEN_INFTY;
	if (!strcasecmp(s->buffer, "NaN"))
		return ISL_TOKEN_NAN;

	if (!s->keywords)
		return ISL_TOKEN_IDENT;

	name_hash = isl_hash_string(isl_hash_init(), s->buffer);
	entry = isl_hash_table_find(s->ctx, s->keywords, name_hash, same_name,
					s->buffer, 0);
	if (entry) {
		keyword = entry->data;
		return keyword->type;
	}

	return ISL_TOKEN_IDENT;
}

static struct isl_token *next_token(struct isl_stream *s, int same_line)
{
	int c;
	struct isl_token *tok = NULL;
	int line, col;
	int old_line = s->line;

	if (s->n_token) {
		if (same_line && s->tokens[s->n_token - 1]->on_new_line)
			return NULL;
		return s->tokens[--s->n_token];
	}

	if (same_line && s->c == '\n')
		return NULL;

	s->len = 0;

	/* skip spaces and comment lines */
	while ((c = isl_stream_getc(s)) != -1) {
		if (c == '#') {
			while ((c = isl_stream_getc(s)) != -1 && c != '\n')
				/* nothing */
				;
			if (c == -1 || (same_line && c == '\n'))
				break;
		} else if (!isspace(c) || (same_line && c == '\n'))
			break;
	}

	line = s->line;
	col = s->col;

	if (c == -1 || (same_line && c == '\n'))
		return NULL;
	if (c == '(' ||
	    c == ')' ||
	    c == '+' ||
	    c == '/' ||
	    c == '*' ||
	    c == '^' ||
	    c == '=' ||
	    c == '@' ||
	    c == ',' ||
	    c == ';' ||
	    c == '[' ||
	    c == ']' ||
	    c == '{' ||
	    c == '}') {
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = (enum isl_token_type)c;
		return tok;
	}
	if (c == '-') {
		int c;
		if ((c = isl_stream_getc(s)) == '>') {
			tok = isl_token_new(s->ctx, line, col, old_line != line);
			if (!tok)
				return NULL;
			tok->type = ISL_TOKEN_TO;
			return tok;
		}
		if (c != -1)
			isl_stream_ungetc(s, c);
		if (!isdigit(c)) {
			tok = isl_token_new(s->ctx, line, col, old_line != line);
			if (!tok)
				return NULL;
			tok->type = (enum isl_token_type) '-';
			return tok;
		}
	}
	if (c == '-' || isdigit(c)) {
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = ISL_TOKEN_VALUE;
		isl_int_init(tok->u.v);
		if (isl_stream_push_char(s, c))
			goto error;
		while ((c = isl_stream_getc(s)) != -1 && isdigit(c))
			if (isl_stream_push_char(s, c))
				goto error;
		if (c != -1)
			isl_stream_ungetc(s, c);
		isl_stream_push_char(s, '\0');
		isl_int_read(tok->u.v, s->buffer);
		return tok;
	}
	if (isalpha(c)) {
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		isl_stream_push_char(s, c);
		while ((c = isl_stream_getc(s)) != -1 && isalnum(c))
			isl_stream_push_char(s, c);
		if (c != -1)
			isl_stream_ungetc(s, c);
		while ((c = isl_stream_getc(s)) != -1 && c == '\'')
			isl_stream_push_char(s, c);
		if (c != -1)
			isl_stream_ungetc(s, c);
		isl_stream_push_char(s, '\0');
		tok->type = check_keywords(s);
		if (tok->type == ISL_TOKEN_IDENT)
			tok->u.s = strdup(s->buffer);
		return tok;
	}
	if (c == ':') {
		int c;
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		if ((c = isl_stream_getc(s)) == '=') {
			tok->type = ISL_TOKEN_DEF;
			return tok;
		}
		if (c != -1)
			isl_stream_ungetc(s, c);
		tok->type = ':';
		return tok;
	}
	if (c == '>') {
		int c;
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		if ((c = isl_stream_getc(s)) == '=') {
			tok->type = ISL_TOKEN_GE;
			return tok;
		}
		if (c != -1)
			isl_stream_ungetc(s, c);
		tok->type = ISL_TOKEN_GT;
		return tok;
	}
	if (c == '<') {
		int c;
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		if ((c = isl_stream_getc(s)) == '=') {
			tok->type = ISL_TOKEN_LE;
			return tok;
		}
		if (c != -1)
			isl_stream_ungetc(s, c);
		tok->type = ISL_TOKEN_LT;
		return tok;
	}
	if (c == '&') {
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = ISL_TOKEN_AND;
		if ((c = isl_stream_getc(s)) != '&' && c != -1)
			isl_stream_ungetc(s, c);
		return tok;
	}
	if (c == '|') {
		tok = isl_token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = ISL_TOKEN_OR;
		if ((c = isl_stream_getc(s)) != '|' && c != -1)
			isl_stream_ungetc(s, c);
		return tok;
	}

	tok = isl_token_new(s->ctx, line, col, old_line != line);
	if (!tok)
		return NULL;
	tok->type = ISL_TOKEN_UNKNOWN;
	return tok;
error:
	isl_token_free(tok);
	return NULL;
}

struct isl_token *isl_stream_next_token(struct isl_stream *s)
{
	return next_token(s, 0);
}

struct isl_token *isl_stream_next_token_on_same_line(struct isl_stream *s)
{
	return next_token(s, 1);
}

int isl_stream_eat_if_available(struct isl_stream *s, int type)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok)
		return 0;
	if (tok->type == type) {
		isl_token_free(tok);
		return 1;
	}
	isl_stream_push_token(s, tok);
	return 0;
}

int isl_stream_next_token_is(struct isl_stream *s, int type)
{
	struct isl_token *tok;
	int r;

	tok = isl_stream_next_token(s);
	if (!tok)
		return 0;
	r = tok->type == type;
	isl_stream_push_token(s, tok);
	return r;
}

char *isl_stream_read_ident_if_available(struct isl_stream *s)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok)
		return NULL;
	if (tok->type == ISL_TOKEN_IDENT) {
		char *ident = strdup(tok->u.s);
		isl_token_free(tok);
		return ident;
	}
	isl_stream_push_token(s, tok);
	return NULL;
}

int isl_stream_eat(struct isl_stream *s, int type)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok)
		return -1;
	if (tok->type == type) {
		isl_token_free(tok);
		return 0;
	}
	isl_stream_error(s, tok, "expecting other token");
	isl_stream_push_token(s, tok);
	return -1;
}

int isl_stream_is_empty(struct isl_stream *s)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);

	if (!tok)
		return 1;

	isl_stream_push_token(s, tok);
	return 0;
}

static int free_keyword(void *p)
{
	struct isl_keyword *keyword = p;

	free(keyword->name);
	free(keyword);

	return 0;
}

void isl_stream_free(struct isl_stream *s)
{
	if (!s)
		return;
	free(s->buffer);
	if (s->n_token != 0) {
		struct isl_token *tok = isl_stream_next_token(s);
		isl_stream_error(s, tok, "unexpected token");
		isl_token_free(tok);
	}
	if (s->keywords) {
		isl_hash_table_foreach(s->ctx, s->keywords, free_keyword);
		isl_hash_table_free(s->ctx, s->keywords);
	}
	isl_ctx_deref(s->ctx);
	free(s);
}
