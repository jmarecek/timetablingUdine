/*
 * Copyright (C) 2002 Sampo Niskanen, Patric Östergård.
 * Minor changes to allow for cross-platform compilation, Jakub Marecek.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

#ifndef CLIQUER_H
#define CLIQUER_H

#include <string.h>

#include "set.h"
#include "graph.h"
#include "reorder.h"

typedef struct _clique_options clique_options;
struct _clique_options {
	int *(*reorder_function)(graph_t *, boolean);
	int *reorder_map;

	/* arguments:  level, n, max, user_time, system_time, opts */
	boolean (*time_function)(int,int,int,int,double,double,
				 clique_options *);
	FILE *output;

	boolean (*user_function)(set_t,graph_t *,clique_options *);
	void *user_data;
	set_t *clique_list;
	int clique_list_length;
};


/* Weighted clique functions */
int clique_max_weight(graph_t *g,clique_options *opts);
set_t clique_find_single(graph_t *g,int min_weight,int max_weight,
				boolean maximal, clique_options *opts);
int clique_find_all(graph_t *g, int req_weight, boolean exact,
			   boolean maximal, clique_options *opts);

/* Unweighted clique functions */
#define clique_unweighted_max_size clique_unweighted_max_weight
int clique_unweighted_max_weight(graph_t *g, clique_options *opts);
set_t clique_unweighted_find_single(graph_t *g,int min_size,
					   int max_size,boolean maximal,
					   clique_options *opts);
int clique_unweighted_find_all(graph_t *g, int min_size, int max_size,
				      boolean maximal, clique_options *opts);

#endif /* !CLIQUER_H */
