
#define _DEFAlast_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include "malloc.h"
#include "printfmt.h"
#include <string.h>
#include <stdlib.h>

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

const size_t SIZE_MINIMO = 256, PEQUENO = 16 * 1024, MEDIANO = 1024 * 1024,
             GRANDE = 32 * 1024 * 1024;

// void coalesce(struct region* left_reg, struct region* right_reg);
// struct region* split_free_regions(struct region *region, size_t size);
struct region *search_last_header(struct region *region);
struct region *split_free_regions(struct region *region_a_splitear, size_t size);
void coalesce_free_regions(struct region *left_reg, struct region *right_reg);
bool is_block_empty(struct region *curr);

void liberar_bloque(struct region *curr);
bool is_block_empty(struct region *curr);
struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
};

struct region_information {
	struct region *first;
	struct region *last;
	size_t memoria_disponible;
};

struct region_information region_info = { .first = NULL,
	                                  .last = NULL,
	                                  .memoria_disponible = 0 };

const size_t TAMANIO_HEADER = sizeof(struct region);


int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

static void
print_statistics(void)
{
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
}

// finds the next free region
// that holds the requested size
//

static struct region *
find_free_region(size_t size)
{
	struct region *region = region_info.first;

#ifdef FIRST_FIT
	while (region != NULL) {
		if (region->free && (region->size >= size))
			return region;

		region = region->next;
	}
#endif

#ifdef BEST_FIT
	struct region *region_menor_tamaño = NULL;
	while (region) {
		if (region->free && (region->size >= size) &&
		    region->size < region_menor_tamaño->size) {
			region_menor_tamaño = region;
		}
		region = region->next;
	}
	return region_menor_tamaño;
#endif

	return NULL;
}

static struct region *
grow_heap(size_t size)
{
	size_t tamanio = 0;

	if (size + TAMANIO_HEADER < PEQUENO)
		tamanio = PEQUENO;
	else if (size + TAMANIO_HEADER < MEDIANO)
		tamanio = MEDIANO;
	else
		tamanio = GRANDE;

	// finds the current heap break
	struct region *curr = (struct region *) sbrk(0);

	// allocates the requested size
	struct region *prev = (struct region *) sbrk(TAMANIO_HEADER + tamanio);

	// verifies that the returned address
	// is the same that the previous break
	// (ref: sbrk(2))
	assert(curr == prev);

	// verifies that the allocation
	// is successful
	//
	// (ref: sbrk(2))
	if (curr == (struct region *) -1) {
		return NULL;
	}
	curr = (struct region *) mmap(NULL,
	                              tamanio,
	                              PROT_READ | PROT_WRITE,
	                              MAP_ANONYMOUS | MAP_PRIVATE,
	                              -1,
	                              0);
	curr->size = tamanio - TAMANIO_HEADER;
	curr->free = true;
	curr->next = NULL;
	curr->prev = NULL;

	// first time here
	if (!region_info.first) {
		region_info.first = curr;
		region_info.memoria_disponible = curr->size;
		atexit(print_statistics);
	} else {
		struct region *last_header = search_last_header(region_info.last);
		last_header->next = curr;
		region_info.memoria_disponible += curr->size;
	}

	region_info.last = curr;
	return curr;
}

struct region *
search_last_header(struct region *region)
{
	if (region->next == NULL ||
	    region->next->prev ==
	            NULL)  //  el firstero del proximo bloque no tiene prev en nuestra implementacion
		return region;
	return search_last_header(region->next);
}


struct region *
split_free_regions(struct region *region_a_splitear, size_t size)
{
	if (((int) region_a_splitear->size - (int) size -
	     (int) sizeof(struct region)) < (int) SIZE_MINIMO) {
		region_info.memoria_disponible -= region_a_splitear->size;
		region_a_splitear->free = false;
		return region_a_splitear;  /// Para que no quede una region con menos del size_minimo
	}

	char *right = (char *) region_a_splitear + sizeof(struct region) +
	              size;  // AGarra la direccion

	((struct region *) right)->size =
	        region_a_splitear->size - size - sizeof(struct region);
	((struct region *) right)->next = region_a_splitear->next;
	((struct region *) right)->prev = region_a_splitear;
	((struct region *) right)->free = true;

	region_a_splitear->size = size;
	region_a_splitear->next = (struct region *) right;
	region_a_splitear->free = false;

	region_info.memoria_disponible -=
	        (region_a_splitear->size + sizeof(struct region));

	return region_a_splitear;
}

void *
malloc(size_t size)
{
	printfmt("malloc con %lu \n", size);
	if (size < SIZE_MINIMO)
		size = SIZE_MINIMO;
	if (size > GRANDE - TAMANIO_HEADER) {
		return NULL;
	}
	size = ALIGN4(size);
	struct region *next = NULL;

	// aligns to mlastiple of 4 bytes
	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	if (region_info.first && region_info.memoria_disponible >= size)
		next = find_free_region(size);
	if (!next) {
		next = grow_heap(size);
	}

	next = split_free_regions(next, size);
	return REGION2PTR(next);
}


void
coalesce_free_regions(struct region *left_reg, struct region *right_reg)
{
	if (right_reg->next && right_reg->next->prev)
		right_reg->next->prev = left_reg;

	left_reg->size += right_reg->size + sizeof(struct region);
	left_reg->next = right_reg->next;
	region_info.memoria_disponible += sizeof(struct region);
}

bool
is_block_empty(struct region *curr)
{
	return !curr->prev && (!curr->next || !curr->next->prev) &&
	       (curr->size == PEQUENO - TAMANIO_HEADER ||
	        curr->size == MEDIANO - TAMANIO_HEADER ||
	        curr->size == GRANDE - TAMANIO_HEADER);
}


void
free(void *ptr)
{
	amount_of_frees++;
	struct region *curr = PTR2REGION(ptr);

	assert(!curr->free);  // Si está liberada, tira error.
	curr->free = true;
	region_info.memoria_disponible += curr->size;

	if (curr->next && curr->next->free)
		coalesce_free_regions(curr, curr->next);
	if (curr->prev && curr->prev->free)
		coalesce_free_regions(curr->prev, curr);
	if (is_block_empty(curr))  // caso de un bloque todo vacio
		liberar_bloque(curr);
}

void
liberar_bloque(struct region *curr)
{
	struct region *actual = region_info.first;
	struct region *ult_bloque = NULL;

	while (actual != curr) {
		ult_bloque = actual;
		actual = actual->next;
	}
	if (ult_bloque) {
		ult_bloque->next = curr->next;
	}
	if (!ult_bloque) {
		region_info.first = curr->next;
		region_info.memoria_disponible = 0;
		if (!curr->next)
			region_info.last = NULL;
	} else
		region_info.memoria_disponible -= curr->size;
	int i = munmap((curr), curr->size + TAMANIO_HEADER);
	if (i != 0) {
		perror("mun map no pudo eliminar el bloque");
		exit(-1);
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if (nmemb == 0 || size == 0)
		return NULL;
	size_t size_real = nmemb * size;
	struct region *region = PTR2REGION(malloc(size_real));
	for (size_t i = 0; i < size_real; i++)
		*((char *) region + TAMANIO_HEADER + i) = 0;
	return REGION2PTR(region);
}


// REALLOC

void *
shrink_region(struct region *region_to_realloc, size_t size)
{
	return split_free_regions(region_to_realloc, size);
}

bool
can_expand_region(struct region *region_to_realloc, size_t size, size_t extra_size)
{
	return region_to_realloc->next && region_to_realloc->next->free &&
	       extra_size < (region_to_realloc->next->size + TAMANIO_HEADER);
}

void *
expand_region(struct region *region_to_realloc, size_t size, size_t extra_size)
{
	size_t extraido = region_to_realloc->next->size;

	coalesce_free_regions(region_to_realloc, region_to_realloc->next);

	region_info.memoria_disponible -=
	        TAMANIO_HEADER;  /// PORQUE COALESCE SE LO SUMA
	region_info.memoria_disponible -= extraido;
	requested_memory += extra_size;
	struct region *misma_region = split_free_regions(region_to_realloc, size);
	return REGION2PTR(misma_region);
}

void *
realloc_to_new_region(struct region *region_to_realloc, size_t size)
{
	void *new_region = malloc(size);

	if (!new_region)
		return NULL;

	memcpy(new_region, region_to_realloc, region_to_realloc->size);
	free(region_to_realloc);

	return new_region;
}

void *
realloc(void *ptr, size_t size)
{
	void *aux;
	size = ALIGN4(size);

	if (!ptr)
		return (malloc(size));
	else if (size == 0) {
		free(ptr);
		return NULL;
	}

	struct region *region_to_realloc = PTR2REGION(ptr);
	size_t tamanio_extra = size - region_to_realloc->size;

	if (size <= region_to_realloc->size)
		return shrink_region(region_to_realloc, size);
	else if (can_expand_region(region_to_realloc, size, tamanio_extra))
		return expand_region(region_to_realloc, size, tamanio_extra);
	else
		return realloc_to_new_region(region_to_realloc, size);
}
