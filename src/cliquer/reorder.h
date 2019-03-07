/*
 * Copyright (C) 2002 Sampo Niskanen, Patric Östergård.
 * Minor changes to allow for cross-platform compilation, Jakub Marecek.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

#ifndef CLIQUER_REORDER_H
#define CLIQUER_REORDER_H

#include "set.h"
#include "graph.h"

void reorder_set(set_t s,int *order);
void reorder_graph(graph_t *g, int *order);
int *reorder_duplicate(int *order,int n);
void reorder_invert(int *order,int n);
void reorder_reverse(int *order,int n);
int *reorder_ident(int n);
boolean reorder_is_bijection(int *order,int n);


#define reorder_by_default reorder_by_greedy_coloring
int *reorder_by_greedy_coloring(graph_t *g, boolean weighted);
int *reorder_by_weighted_greedy_coloring(graph_t *g, boolean weighted);
int *reorder_by_unweighted_greedy_coloring(graph_t *g,boolean weighted);
int *reorder_by_degree(graph_t *g, boolean weighted);
int *reorder_by_random(graph_t *g, boolean weighted);
int *reorder_by_ident(graph_t *g, boolean weighted);
int *reorder_by_reverse(graph_t *g, boolean weighted);

#endif /* !CLIQUER_REORDER_H */
