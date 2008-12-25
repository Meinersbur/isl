#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <isl_seq.h>
#include "isl_map_private.h"
#include "isl_input_omega.h"

enum token_type { TOKEN_UNKNOWN = 256, TOKEN_VALUE, TOKEN_IDENT, TOKEN_GE,
		  TOKEN_LE, TOKEN_TO, TOKEN_AND, TOKEN_EXISTS };

struct token {
	enum token_type  type;

	unsigned int on_new_line : 1;
	int line;
	int col;

	union {
		isl_int	v;
		char	*s;
	} u;
};

static struct token *token_new(struct isl_ctx *ctx,
	int line, int col, unsigned on_new_line)
{
	struct token *tok = isl_alloc_type(ctx, struct token);
	if (!tok)
		return NULL;
	tok->line = line;
	tok->col = col;
	tok->on_new_line = on_new_line;
	return tok;
}

void token_free(struct token *tok)
{
	if (!tok)
		return;
	if (tok->type == TOKEN_VALUE)
		isl_int_clear(tok->u.v);
	else if (tok->type == TOKEN_IDENT)
		free(tok->u.s);
	free(tok);
}

struct stream {
	struct isl_ctx	*ctx;
	FILE        	*file;
	const char  	*str;
	int	    	line;
	int	    	col;
	int	    	eof;

	char	    	*buffer;
	size_t	    	size;
	size_t	    	len;
	int	    	c;

	struct token	*tokens[5];
	int	    	n_token;
};

void stream_error(struct stream *s, struct token *tok, char *msg)
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

static void stream_free(struct stream *s);

static struct stream* stream_new(struct isl_ctx *ctx)
{
	int i;
	struct stream *s = isl_alloc_type(ctx, struct stream);
	if (!s)
		return NULL;
	s->ctx = ctx;
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
	return s;
error:
	stream_free(s);
	return NULL;
}

static struct stream* stream_new_file(struct isl_ctx *ctx, FILE *file)
{
	struct stream *s = stream_new(ctx);
	if (!s)
		return NULL;
	s->file = file;
	return s;
}

static struct stream* stream_new_str(struct isl_ctx *ctx, const char *str)
{
    struct stream *s = stream_new(ctx);
    s->str = str;
    return s;
}

static int stream_getc(struct stream *s)
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

static void stream_ungetc(struct stream *s, int c)
{
	if (s->file)
		ungetc(c, s->file);
	else
		--s->str;
	s->c = -1;
}

static int stream_push_char(struct stream *s, int c)
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

static void stream_push_token(struct stream *s, struct token *tok)
{
	isl_assert(s->ctx, s->n_token < 5, return);
	s->tokens[s->n_token++] = tok;
}

static struct token *stream_next_token(struct stream *s)
{
	int c;
	struct token *tok = NULL;
	int line, col;
	int old_line = s->line;

	if (s->n_token)
		return s->tokens[--s->n_token];

	s->len = 0;

	/* skip spaces */
	while ((c = stream_getc(s)) != -1 && isspace(c))
		/* nothing */
		;

	line = s->line;
	col = s->col;

	if (c == -1)
		return NULL;
	if (c == '(' ||
	    c == ')' ||
	    c == '+' ||
	    c == '/' ||
	    c == '*' ||
	    c == '^' ||
	    c == '=' ||
	    c == ',' ||
	    c == ':' ||
	    c == '[' ||
	    c == ']' ||
	    c == '{' ||
	    c == '}') {
		tok = token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = (enum token_type)c;
		return tok;
	}
	if (c == '-') {
		int c;
		if ((c = stream_getc(s)) == '>') {
			tok = token_new(s->ctx, line, col, old_line != line);
			if (!tok)
				return NULL;
			tok->type = TOKEN_TO;
			return tok;
		}
		if (c != -1)
			stream_ungetc(s, c);
	}
	if (c == '-' || isdigit(c)) {
		tok = token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = TOKEN_VALUE;
		isl_int_init(tok->u.v);
		if (stream_push_char(s, c))
			goto error;
		while ((c = stream_getc(s)) != -1 && isdigit(c))
			if (stream_push_char(s, c))
				goto error;
		if (c != -1)
			stream_ungetc(s, c);
		if (s->len == 1 && s->buffer[0] == '-')
			isl_int_set_si(tok->u.v, -1);
		else {
			stream_push_char(s, '\0');
			isl_int_read(tok->u.v, s->buffer);
		}
		return tok;
	}
	if (isalpha(c)) {
		tok = token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		stream_push_char(s, c);
		while ((c = stream_getc(s)) != -1 && isalnum(c))
			stream_push_char(s, c);
		if (c != -1)
			stream_ungetc(s, c);
		stream_push_char(s, '\0');
		if (!strcasecmp(s->buffer, "exists"))
			tok->type = TOKEN_EXISTS;
		else {
			tok->type = TOKEN_IDENT;
			tok->u.s = strdup(s->buffer);
		}
		return tok;
	}
	if (c == '>') {
		int c;
		if ((c = stream_getc(s)) == '=') {
			tok = token_new(s->ctx, line, col, old_line != line);
			if (!tok)
				return NULL;
			tok->type = TOKEN_GE;
			return tok;
		}
		if (c != -1)
			stream_ungetc(s, c);
	}
	if (c == '<') {
		int c;
		if ((c = stream_getc(s)) == '=') {
			tok = token_new(s->ctx, line, col, old_line != line);
			if (!tok)
				return NULL;
			tok->type = TOKEN_LE;
			return tok;
		}
		if (c != -1)
			stream_ungetc(s, c);
	}
	if (c == '&') {
		tok = token_new(s->ctx, line, col, old_line != line);
		if (!tok)
			return NULL;
		tok->type = TOKEN_AND;
		if ((c = stream_getc(s)) != '&' && c != -1)
			stream_ungetc(s, c);
		return tok;
	}

	tok = token_new(s->ctx, line, col, old_line != line);
	if (!tok)
		return NULL;
	tok->type = TOKEN_UNKNOWN;
	return tok;
error:
	token_free(tok);
	return NULL;
}

static int stream_eat(struct stream *s, int type)
{
	struct token *tok;

	tok = stream_next_token(s);
	if (!tok)
		return -1;
	if (tok->type == type) {
		token_free(tok);
		return 0;
	}
	stream_error(s, tok, "expecting other token");
	stream_push_token(s, tok);
	return -1;
}

static void stream_free(struct stream *s)
{
	if (!s)
		return;
	free(s->buffer);
	if (s->n_token != 0) {
		struct token *tok = stream_next_token(s);
		stream_error(s, tok, "unexpected token");
		token_free(tok);
	}
	free(s);
}

struct variable {
	char    	    	*name;
	int	     		 pos;
	struct variable		*next;
};

struct vars {
	struct isl_ctx	*ctx;
	int		 n;
	struct variable	*v;
};

static struct vars *vars_new(struct isl_ctx *ctx)
{
	struct vars *v;
	v = isl_alloc_type(ctx, struct vars);
	if (!v)
		return NULL;
	v->ctx = ctx;
	v->n = 0;
	v->v = NULL;
	return v;
}

void variable_free(struct variable *var)
{
	while (var) {
		struct variable *next = var->next;
		free(var->name);
		free(var);
		var = next;
	}
}

static void vars_free(struct vars *v)
{
	if (!v)
		return;
	variable_free(v->v);
	free(v);
}

struct variable *variable_new(struct vars *v, const char *name, int len,
				int pos)
{
	struct variable *var;
	var = isl_alloc_type(v->ctx, struct variable);
	if (!var)
		goto error;
	var->name = strdup(name);
	var->name[len] = '\0';
	var->pos = pos;
	var->next = v->v;
	return var;
error:
	variable_free(v->v);
	return NULL;
}

static int vars_pos(struct vars *v, const char *s, int len)
{
	int pos;
	struct variable *q;

	if (len == -1)
		len = strlen(s);
	for (q = v->v; q; q = q->next) {
		if (strncmp(q->name, s, len) == 0 && q->name[len] == '\0')
			break;
	}
	if (q)
		pos = q->pos;
	else {
		pos = v->n;
		v->v = variable_new(v, s, len, v->n);
		if (!v)
			return -1;
		v->n++;
	}
	return pos;
}

static struct vars *read_var_list(struct stream *s, struct vars *v)
{
	struct token *tok;

	while ((tok = stream_next_token(s)) != NULL) {
		int p;
		int n = v->n;

		if (tok->type != TOKEN_IDENT)
			break;

		p = vars_pos(v, tok->u.s, -1);
		if (p < 0)
			goto error;
		if (p < n) {
			stream_error(s, tok, "expecting unique identifier");
			goto error;
		}
		token_free(tok);
		tok = stream_next_token(s);
		if (!tok || tok->type != ',')
			break;

		token_free(tok);
	}
	if (tok)
		stream_push_token(s, tok);

	return v;
error:
	token_free(tok);
	vars_free(v);
	return NULL;
}

static struct vars *read_tuple(struct stream *s, struct vars *v)
{
	struct token *tok;

	tok = stream_next_token(s);
	if (!tok || tok->type != '[') {
		stream_error(s, tok, "expecting '['");
		goto error;
	}
	token_free(tok);
	v = read_var_list(s, v);
	tok = stream_next_token(s);
	if (!tok || tok->type != ']') {
		stream_error(s, tok, "expecting ']'");
		goto error;
	}
	token_free(tok);

	return v;
error:
	if (tok)
		token_free(tok);
	vars_free(v);
	return NULL;
}

static struct isl_basic_map *add_constraints(struct stream *s,
	struct vars **v, struct isl_basic_map *bmap);

static struct isl_basic_map *add_exists(struct stream *s,
	struct vars **v, struct isl_basic_map *bmap)
{
	struct token *tok;
	int n = (*v)->n;
	int extra;
	int seen_paren = 0;
	int i;
	unsigned total;

	tok = stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == '(') {
		seen_paren = 1;
		token_free(tok);
	} else
		stream_push_token(s, tok);
	*v = read_var_list(s, *v);
	if (!*v)
		goto error;
	extra = (*v)->n - n;
	bmap = isl_basic_map_extend_dim(bmap, isl_dim_copy(bmap->dim),
			extra, 0, 0);
	total = isl_basic_map_total_dim(bmap);
	for (i = 0; i < extra; ++i) {
		int k;
		if ((k = isl_basic_map_alloc_div(bmap)) < 0)
			goto error;
		isl_seq_clr(bmap->div[k], 1+1+total);
	}
	if (!bmap)
		return NULL;
	if (stream_eat(s, ':'))
		goto error;
	bmap = add_constraints(s, v, bmap);
	if (seen_paren && stream_eat(s, ')'))
		goto error;
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *add_constraint(struct stream *s,
	struct vars **v, struct isl_basic_map *bmap)
{
	unsigned total = isl_basic_map_total_dim(bmap);
	int k;
	int sign = 1;
	int equality = 0;
	int op = 0;
	struct token *tok = NULL;

	tok = stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == TOKEN_EXISTS) {
		token_free(tok);
		return add_exists(s, v, bmap);
	}
	stream_push_token(s, tok);

	bmap = isl_basic_map_extend_constraints(bmap, 0, 1);
	k = isl_basic_map_alloc_inequality(bmap);
	if (k < 0)
		goto error;
	isl_seq_clr(bmap->ineq[k], 1+total);

	for (;;) {
		tok = stream_next_token(s);
		if (!tok) {
			stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type == TOKEN_IDENT) {
			int n = (*v)->n;
			int pos = vars_pos(*v, tok->u.s, -1);
			if (pos < 0)
				goto error;
			if (pos >= n) {
				stream_error(s, tok, "unknown identifier");
				goto error;
			}
			if (sign > 0)
				isl_int_add_ui(bmap->ineq[k][1+pos],
						bmap->ineq[k][1+pos], 1);
			else
				isl_int_sub_ui(bmap->ineq[k][1+pos],
						bmap->ineq[k][1+pos], 1);
		} else if (tok->type == TOKEN_VALUE) {
			struct token *tok2;
			int n = (*v)->n;
			int pos = -1;
			tok2 = stream_next_token(s);
			if (tok2 && tok2->type == TOKEN_IDENT) {
				pos = vars_pos(*v, tok2->u.s, -1);
				if (pos < 0)
					goto error;
				if (pos >= n) {
					stream_error(s, tok2,
						"unknown identifier");
					token_free(tok2);
					goto error;
				}
				token_free(tok2);
			} else if (tok2)
				stream_push_token(s, tok2);
			if (sign < 0)
				isl_int_neg(tok->u.v, tok->u.v);
			isl_int_add(bmap->ineq[k][1+pos],
					bmap->ineq[k][1+pos], tok->u.v);
		} else if (tok->type == TOKEN_LE) {
			op = 1;
			isl_seq_neg(bmap->ineq[k], bmap->ineq[k], 1+total);
		} else if (tok->type == TOKEN_GE) {
			op = 1;
			sign = -1;
		} else if (tok->type == '=') {
			if (op) {
				stream_error(s, tok, "too many operators");
				goto error;
			}
			op = 1;
			equality = 1;
			sign = -1;
		} else {
			stream_push_token(s, tok);
			break;
		}
		token_free(tok);
	}
	tok = NULL;
	if (!op) {
		stream_error(s, tok, "missing operator");
		goto error;
	}
	if (equality)
		isl_basic_map_inequality_to_equality(bmap, k);
	return bmap;
error:
	if (tok)
		token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *add_constraints(struct stream *s,
	struct vars **v, struct isl_basic_map *bmap)
{
	struct token *tok;

	for (;;) {
		bmap = add_constraint(s, v, bmap);
		if (!bmap)
			return NULL;
		tok = stream_next_token(s);
		if (!tok) {
			stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type != TOKEN_AND)
			break;
		token_free(tok);
	}
	stream_push_token(s, tok);

	return bmap;
error:
	if (tok)
		token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *basic_map_read(struct stream *s)
{
	struct isl_basic_map *bmap = NULL;
	struct token *tok;
	struct vars *v = NULL;
	int n1;
	int n2;

	tok = stream_next_token(s);
	if (!tok || tok->type != '{') {
		stream_error(s, tok, "expecting '{'");
		if (tok)
			stream_push_token(s, tok);
		goto error;
	}
	token_free(tok);
	v = vars_new(s->ctx);
	v = read_tuple(s, v);
	if (!v)
		return NULL;
	n1 = v->n;
	tok = stream_next_token(s);
	if (tok && tok->type == TOKEN_TO) {
		token_free(tok);
		v = read_tuple(s, v);
		if (!v)
			return NULL;
		n2 = v->n - n1;
	} else {
		if (tok)
			stream_push_token(s, tok);
		n2 = n1;
		n1 = 0;
	}
	bmap = isl_basic_map_alloc(s->ctx, 0, n1, n2, 0, 0,0);
	if (!bmap)
		goto error;
	tok = stream_next_token(s);
	if (tok && tok->type == ':') {
		token_free(tok);
		bmap = add_constraints(s, &v, bmap);
		tok = stream_next_token(s);
	}
	if (tok && tok->type == '}') {
		token_free(tok);
	} else {
		stream_error(s, tok, "unexpected token");
		if (tok)
			token_free(tok);
		goto error;
	}
	vars_free(v);

	return bmap;
error:
	isl_basic_map_free(bmap);
	if (v)
		vars_free(v);
	return NULL;
}

struct isl_basic_map *isl_basic_map_read_from_file_omega(
		struct isl_ctx *ctx, FILE *input)
{
	struct isl_basic_map *bmap;
	struct stream *s = stream_new_file(ctx, input);
	if (!s)
		return NULL;
	bmap = basic_map_read(s);
	stream_free(s);
	return bmap;
}

struct isl_basic_set *isl_basic_set_read_from_file_omega(
		struct isl_ctx *ctx, FILE *input)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_read_from_file_omega(ctx, input);
	if (!bmap)
		return NULL;
	isl_assert(ctx, isl_basic_map_n_in(bmap) == 0, goto error);
	return (struct isl_basic_set *)bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_read_from_str_omega(
		struct isl_ctx *ctx, const char *str)
{
	struct isl_basic_map *bmap;
	struct stream *s = stream_new_str(ctx, str);
	if (!s)
		return NULL;
	bmap = basic_map_read(s);
	stream_free(s);
	return bmap;
}

struct isl_basic_set *isl_basic_set_read_from_str_omega(
		struct isl_ctx *ctx, const char *str)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_read_from_str_omega(ctx, str);
	if (!bmap)
		return NULL;
	isl_assert(ctx, isl_basic_map_n_in(bmap) == 0, goto error);
	return (struct isl_basic_set *)bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}
