#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "attr.h"
#include "mapfile.h"
#include "template.h"

struct entry {
	unsigned type; /* 0=raw 1=attribute */
	unsigned len;
	const char *data;
	struct entry *next;
};

struct template {
	struct mapfile_info mi;
	struct entry *entry_list;
};

static struct entry *create_entry(unsigned type, const char *data, unsigned len) {
	struct entry *ret;
	ret=malloc(sizeof *ret);
	ret->type=type;
	ret->len=len;
	ret->data=data;
	ret->next=0;
	return ret;
}

static struct entry *parse_string(const char *str, unsigned len) {
	struct entry *ret=0, **curr=&ret;
	const char *end;

	assert(str!=NULL);

	while(len && (end=memchr(str, '$', len))) {
		char ch;
		/* first check for $$ */
		if(len && str[0]=='$' && str[1]=='$') {
			/* we have a $$, fine the next $ so we can setup a run of raw data
			 * in the next condition */
			end=memchr(str+2, '$', len-2);
			if(!end) { /* no second $ found */
				str++;
				len--;
				break; /* use check after loop to save string */
			}
			/* this will fall into the next condition because end!=str */
		}
		/* str to end is "raw" data */
		if(end!=str) {
			unsigned sublen;
			sublen=end-str;
			/* found $$ near the end of our rawstring */
			if(len>=sublen+2 && end[1]=='$') {
				sublen++;
				end+=2;
			}
			/* look for $ at end of string and add it to the end */
			if(len==sublen+1 && *end=='$') {
				sublen++;
				end++;
			}
			/* TODO: look for $$ at the end and add $ in that case */
			*curr=create_entry(0, str, sublen);
			curr=&(*curr)->next;
			len-=end-str;
			str=end;
		}
		if(!len) break; /* used up all the data */

		/* str and end is "${...}" */
		assert(str==end);
		assert(*end=='$');
		str++;
		len--;
		ch=*str;
		if(len && (ch=='(' || ch=='{')) {
			/* $(...) or ${...} found */
			str++;
			len--;
			end=memchr(str, ch=='('?')':'}' , len);
			if(end) {
				*curr=create_entry(1, str, end-str);
				curr=&(*curr)->next;
				if(len) end++; /* eat ) or } */
				len-=end-str;
				str=end;
			} else {
				/* closing ) or } not found. treat as raw */
				/* TODO: if prev is a type 0 then append to it */
				*curr=create_entry(0, str-2, 2);
				curr=&(*curr)->next;
			}
		} else {
			/* $XXX found */
			for(end=str;(end-str)<len && (isalnum(*end) || *end=='_');end++) ;
			if((end-str)>1) {
				*curr=create_entry(1, str, end-str);
				curr=&(*curr)->next;
			} else {
				/* $ at end of string */
				*curr=create_entry(0, str-1, end-str+1);
				curr=&(*curr)->next;
			}
			len-=end-str;
			str=end;
		}
	}
	if(len) { /* use the remaining as raw data */
		*curr=create_entry(0, str, len);
		curr=&(*curr)->next;
	}
	return ret;
}

struct template *template_loadfile(const char *filename) {
	struct template *ret;
	ret=malloc(sizeof *ret);
	if(!mapfile(&ret->mi, filename)) {
		free(ret);
		return 0; /* failure */
	}
	ret->entry_list=parse_string(ret->mi.data, ret->mi.len);
	return ret;
}

/* al - NULL to use a "dumping" mode */
void template_apply(struct template *t, attrlist_t al, FILE *out) {
	struct entry *e;
	char buf[64]; /* max macro length */
	const char *tmp;

	/* TODO: we could cache the attrget lookup */

	for(e=t->entry_list;e;e=e->next) {
		/* used for debugging :
		 * fprintf(stderr, "::%u:%u:%.*s\n", e->type, e->len, e->len, e->data);
		 */
		/* ignore macro type if the macro is too large */
		if(al && e->type && (e->len<(sizeof buf-1))) {
			memcpy(buf, e->data, e->len);
			buf[e->len]=0;
			tmp=attrget(al, buf);
			if(tmp) {
				fputs(tmp, out);
			}
		} else if(!al && e->type) {
			fprintf(out, "${%.*s}", e->len, e->data);
		} else {
			fprintf(out, "%.*s", e->len, e->data);
		}
	}
}

void template_free(struct template *t) {
	struct entry *curr, *next;
	for(curr=t->entry_list;curr;curr=next) {
		next=curr->next;
		free(curr);
	}
	mapfile_release(&t->mi);
	free(t);
}

