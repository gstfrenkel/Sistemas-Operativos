#define FUSE_USE_VERSION 30
#define _GNU_SOURCE
#define __USE_GNU
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <utime.h>
#include <time.h>
#define MAX_CONTENT 1000
#define MAX_NAME 20
#define MAX_INODE 4 * 100
#define MAX_PATH 100
#define MAX_CHILDREN 20
#define MAX_FILE_PATH 200
#define MAX_LINE 200


struct inode {
	struct stat stats;  // size, ctime, mtime, mode, uid, gid
	int dir;  // 0 archivo 1 directory                               4bytes
	struct inode *parent;                 //    8bytes
	char name[MAX_NAME];                  //    40bytes
	char data[MAX_CONTENT];               //    1000bytes
	int children_position[MAX_CHILDREN];  //   2200bytes -> 2kb
	size_t children_amount;
	char path[MAX_PATH];
} inode_t;

struct super {
	struct inode inode_array[MAX_INODE];
	int bitmap[MAX_INODE];  // 0 usado 1 disponible
} super_t;
struct super super = {};

char file_path[MAX_FILE_PATH];
char NAME_PERSISTENCE[MAX_NAME];

struct timespec
get_time()
{
	struct timespec time;
	timespec_get(&time, TIME_UTC);
	return time;
}

struct inode *
search_inode_by_path(const char *path)
{
	size_t i = 0;
	while (i < MAX_INODE) {
		if (super.bitmap[i] == 0) {
			if (!strcmp(super.inode_array[i].path,
			            path)) {  /// encontramos un inodo con path absoluto iguales
				if (i != 0) {
					struct inode *parent =
					        super.inode_array[i].parent;
					parent->stats.st_atim = get_time();
				}
				return &super.inode_array[i];
			}
		}
		i++;
	}
	return NULL;
}

 /// hardcodeamos el size del directorio como se dijo en clase, no recorremos recursivamente el tamaño de cada file dentro del directorio                                        
const size_t SIZE_DIRECTORY = sizeof(inode_t);  // size directory

void
update_getattr_stats(struct stat *st, struct stat inode_st)
{
	st->st_uid = inode_st.st_uid;
	st->st_mode = inode_st.st_mode;
	st->st_nlink = inode_st.st_nlink;
	st->st_ctim = inode_st.st_ctim;
	st->st_mtim = inode_st.st_mtim;
	st->st_atim = inode_st.st_atim;
	st->st_size = inode_st.st_size;
}


static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	struct inode *inode = search_inode_by_path(path);
	if (inode == NULL) 	return -ENOENT;


	update_getattr_stats(st, inode->stats);
	return 0;
}

/// a partir del path que se nos pasa, verificamos de que sea un inodo directorio y luego llenamos el buffer con sus hijos, que los encontramos a partir de
/// la posicion de ellos dentro del arreglo de inodos del super.
static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);
	struct inode *inode = search_inode_by_path(path);

	if (!inode)
		return -ENOENT;

	if (!inode->dir) {
		filler(buffer, inode->name, NULL, 0);
		return 0;
	}

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	for (int i = 0; i < inode->children_amount; i++) {
		int inode_position = inode->children_position[i];
		struct inode children_inode = super.inode_array[inode_position];
		filler(buffer, children_inode.name, NULL, 0);
	}

	return 0;
}


/// chequeamos que se pueda leer y a partir de eso se lee la data del file, actualizando su respectivo stat y devolviendo la cantidad leida.
static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu) \n", path, offset, size);

	struct inode *inode_to_read = search_inode_by_path(path);

	if (!inode_to_read)
		return -ENOENT;
	if (inode_to_read->dir == 1)
		return -EISDIR;
	if (inode_to_read->stats.st_mode != 33277 &&
	    inode_to_read->stats.st_mode != 33204 &&
	    inode_to_read->stats.st_mode != 33188 &&
	    inode_to_read->stats.st_mode != 33060)
		return -EACCES;

	char *fisop_file_contenidos = inode_to_read->data;

	if (offset + size > strlen(fisop_file_contenidos))
		size = strlen(fisop_file_contenidos) - offset;

	size = size > 0 ? size : 0;

	inode_to_read->stats.st_atim = get_time();

	strncpy(buffer, fisop_file_contenidos + offset, size);
	return size;
}

size_t
search_empty_inode()
{
	size_t i = 0;

	for (size_t i = 0; i < MAX_INODE; i++)
		if (super.bitmap[i] != 0)
			return i;
	while (i < MAX_INODE) {
		if (super.bitmap[i])
			return i;
		i++;
	}
	return 0;
}

/// A partir del arreglo bitmap chequeamos si un inodo se encuentra ocupado y matchea el path absoluto.
int inode_exists(const char* abs_path) {
	for(int i = 1; i < MAX_INODE; i++) {
		if (super.bitmap[i] == 0 && strcmp(super.inode_array[i].path , abs_path) == 0) {

			return 1;
		}
	}
	return 0;
}


// Encuentra un inodo y setea sus respectivos stats.
void
get_new_inode(size_t position,
              struct inode *parent_inode,
              const char *name,
              const char *abs_path,
              __mode_t st_mode,
              int is_dir)
{
	struct inode *inode = &super.inode_array[position];

	strcpy(inode->name, name);
	inode->parent = parent_inode;
	inode->stats.st_mode = st_mode;
	inode->stats.st_nlink = 2;
	inode->stats.st_ctim = get_time();
	inode->stats.st_mtim = get_time();
	inode->stats.st_atim = get_time();
	inode->stats.st_uid = getuid();
	inode->stats.st_gid = getgid();
	inode->children_amount = 0;
	inode->dir = is_dir;
	strcpy(inode->path, abs_path);

	if (is_dir == 1)
		inode->stats.st_size = sizeof(inode_t);
	else
		inode->stats.st_size = 0;

	super.bitmap[position] = 0;
}

/// Recibe el nombre, el padre, el path absoluto y los flags para crear un nuevo directorio.
int
create_directory(const char *name,
                 struct inode *parent,
                 const char *abs_path,
                 mode_t flags)
{
	printf("[debug]  fisop_create_directory(%s)\n", name);
	if (inode_exists(abs_path))
		return -EEXIST;
	// chequea si es root, root está siempre en 0.

	if (!strcmp(name, "/\0"))
		return 0;
	size_t offset = 0;

	while (name[offset] != '/' && name[offset] != '\0') {
		offset++;
	}
	size_t position = search_empty_inode();
	if (!position)
		return -ENOMEM;

	get_new_inode(position, parent, name, abs_path, flags, 1);

	super.inode_array[0].children_position[super.inode_array[0].children_amount] =
	        position;
	super.inode_array[0].children_amount++;

	return 0;
}

int
fisops_create_directory(const char *path, mode_t flags)
{  //
	printf("[debug] fisopfs_mkdir(%s)\n", path);
	struct inode inode_root = super.inode_array[0];
	return create_directory(path + 1, &inode_root, path, __S_IFDIR | 0755);
}

/// a partir del path absoluto se chequea que se pueda eliminar un directorio
int 
fisops_remove_directory(const char *path){
	printf("[debug] fisopfs_remove_directory:(%s)\n", path);
	if (strcmp(path, "/") == 0)
		return EBUSY;  // Nos lo devuelve en nuestras computadoras
	for (int i = 1; i < MAX_INODE; i++) {
		if (super.bitmap[i] == 0 &&
		    !strcmp(path, super.inode_array[i].path)) {
			if (!super.inode_array[i].dir) {
				return -ENOTDIR;
			}
			if (super.inode_array[i].children_amount > 0) {
				return -EPERM;  // En nuestras computadoras no
				                // nos deja borrar un directorio que contenga hijos
			}
			super.bitmap[i] = 1;

			/// como siempre elimino directorios de root, busco la
			/// posicion dentro de children_position que tiene la posicion
			/// del directorio eliminado y la reemplazo por la posición del ultimo hijo.
			struct inode *root = &super.inode_array[0];
			for (int j = 0; j < root->children_amount; j++) {
				if (root->children_position[j] == i) {
					root->children_position[j] =
					        root->children_position[root->children_amount -
					                                1];
				}
			}
			root->children_amount--;

			return 0;
		}
	}
	return -ENOENT;
}

int
fisops_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);
	if (strcmp(path, "/") == 0)
		return EBUSY;  // Nos lo devuelve en nuestras computadoras
	for (int i = 1; i < MAX_INODE; i++) {
		if (super.bitmap[i] == 0 &&
		    !strcmp(path, super.inode_array[i].path)) {
			if (super.inode_array[i].dir == 1) {
				return -EISDIR;
			}
			super.bitmap[i] = 1;
			/// como siempre elimino directorios de root, busco la
			/// posicion dentro de children_position que tiene la posicion
			/// del directorio eliminado y la reemplazo por la posición del ultimo hijo.
			struct inode *parent = super.inode_array[i].parent;
			for (int j = 0; j < parent->children_amount; j++) {
				if (parent->children_position[j] == i) {
					parent->children_position[j] =
					        parent->children_position[parent->children_amount -
					                                  1];
				}
			}
			parent->children_amount--;
			return 0;
		}
	}
	return -ENOENT;
}

/// Se crea un file usando el path absoluto y su modo.
int 
fisops_create_file(const char* path , mode_t mode, struct fuse_file_info* fuse_info) {	
	//chequea si es root, root está siempre en 0.
	printf("[debug] fisopfs_touch:(%s), %u\n", path,mode);
	if (!strcmp(path,"/\0")) return 0;
	
	if (inode_exists(path)) return -EEXIST;
	

	size_t last_slash = 0, i = 0;
	// nos guardamos la ultima posicion del slash para poder guardar bien el padre
	while (path[i] != '\0') {
		if (path[i] == '/') 
			last_slash = i;
		i++;
	}
	// el nombre lo guardamos a partir del last slash: si es 0 entonces es
	// un path de tipo /archivo.txt entonces el nombre sera archivo.txt sino, sera del tipo /directorio/archivo.txt
	char *name = (char *) path + last_slash + 1;
	char parent_path[MAX_PATH];  // \0

	// diferenciamos los dos casos, si queremos el path del padre, si es /
	// queremos que sea +1 asi nos lo agarra, si es /directorio/archivo queremos que nos agarre /directorio
	last_slash = (last_slash == 0 ? last_slash + 1 : last_slash);
	strncpy(parent_path, path, last_slash);
	parent_path[last_slash] = '\0';

	// A partir del path buscamos al padre
	// por ejemplo si es : /archivo.txt el padre es / , y si es
	// /nuevo/archivo.txt el padre es /nuevo con path absoluto
	struct inode *parent = search_inode_by_path(parent_path);
	if (!parent)
		return -ENOENT;
	parent->stats.st_mtim = get_time();

	// chequear si existe el archivo en el directorio actual     /nuevo
	for (int i = 0; i < parent->children_amount; i++) {
		struct inode child =
		        super.inode_array[parent->children_position[i]];
		if (!strcmp(child.path, path) && child.dir == 0)
			return -EEXIST;
	}

	// Chequeamos si hay memoria disponible
	size_t position = search_empty_inode();
	if (!position)
		return -ENOMEM;

	get_new_inode(position, parent, name, path, mode, 0);

	// aumentar el children del padre
	parent->children_position[parent->children_amount] = position;
	parent->children_amount++;
	return 0;
}
int
fisops_write_file(const char *path,
                  const char *data,
                  size_t size_data,
                  off_t offset,
                  struct fuse_file_info *fuse_info)
{
	printf("[debug] fisops_write (%s) \n", path);
	size_t sum = offset + size_data;
	if ((sum) > MAX_CONTENT) return -EFBIG;

	struct inode *inode = search_inode_by_path(path);

	if (!inode) {

		int result = fisops_create_file(
		        path, 33204, fuse_info);  // hay que ver el mode

		if (result < 0)
			return result;
		inode = search_inode_by_path(path);
	}
	if (!inode)
		return -ENOENT;
	if (inode->stats.st_mode != 33277 && inode->stats.st_mode != 33204)
		return -EACCES;


	strcpy(inode->data + sum - size_data, data);
	inode->data[sum] = '\0';
	inode->stats.st_mtim = get_time();
	inode->stats.st_atim = get_time();
	inode->stats.st_size = sum;
	struct inode *parent = inode->parent;
	parent->stats.st_mtim = get_time();

	return (int) size_data;
}


/// Se usa para alocar mas memoria por si tenemos mas data de la que teniamos.
int 
fisops_truncate (const char *path, off_t new_size){
	printf("[debug] fisopfs_truncate(%s)\n", path);
	struct inode *inode = search_inode_by_path(path);
	if (!inode)
		return -ENOENT;
	if (inode->stats.st_mode != 33277 && inode->stats.st_mode != 33204)
		return -EACCES;

	if (inode->stats.st_size - new_size <
	    0) {  // El nuevo size que le quiero poner al archivo es mas grande
		  // del que tengo, tengo que buscar mas memoria
		if (!memset(inode,
		            inode->stats.st_size,
		            new_size - inode->stats.st_size))
			return -ENOMEM;
		return 0;
	}
	inode->stats.st_size = new_size;
	return 0;
}


/// Cambiamos las estadisticas de acceso y modificacion y le cambiamos el nombre a un archivo.
int fisops_rename(const char* path, const char* new_name){
	printf("[debug] fisopfs_rename(%s)\n", path);
	struct inode *inode = search_inode_by_path(path);
	if (!inode)
		return -ENOENT;
	if (strlen(new_name) > MAX_NAME)
		return -E2BIG;
	strcpy(inode->name, new_name);
	inode->stats.st_mtim = get_time();
	inode->stats.st_atim = get_time();
	return 0;
}



int
fisops_utimens(const char *path, const struct timespec tv[2])
{
	struct inode *inode = search_inode_by_path(path);
	if (!inode)
		return -ENOENT;
	inode->stats.st_atim = tv[0];
	inode->stats.st_mtim = tv[1];
	return 0;
}


/// A partir de la informacion de todos los inodos, escribimos en un archivo txt la estructura de los inodos para cargarla luego.
void fisops_destroy(void* ptr){
	printf("[debug] fisops_destroy\n");
	FILE *file = fopen("base.fisops", "w");
	if (!file)
		return;

	for (size_t i = 0; i < MAX_INODE; i++) {
		char aux[MAX_LINE];
		if (!super.bitmap[i]) {
			struct inode inode_to_write = super.inode_array[i];
			if (inode_to_write.dir) {
				strcpy(aux, inode_to_write.path);
				fprintf(file,
				        "%s\n",
				        aux);  // solo escribimos los directorios
				               // primero, para no tener problema
				               // si tenemos un archivo con path /directorio/archivo.txt
			}
		}
	};

	for (size_t i = 0; i < MAX_INODE; i++) {
		struct inode inode_to_write = super.inode_array[i];
		if (!super.bitmap[i] && !inode_to_write.dir) {
			fprintf(file,
			        ";\n%s:%s\n",
			        inode_to_write.path,
			        inode_to_write.data);
		}
	};
	fprintf(file, ";");
	fclose(file);
}

int 
fisops_flush()
{
	return fisops_destroy(NULL);
}

/// a partir de un file se obtienen toda la data y se crean los archivos inicializados.
int 
read_data (FILE* file){
	char aux = fgetc(file);
	char caracter_anterior = aux;
	bool read_path = false;
	int posicion_path = 0;
	int posicion_data = 0;
	char path[MAX_PATH];
	char data[MAX_CONTENT];
	if (aux == '\n') {
		aux = fgetc(file);
	}
	while (!feof(file) && !(!(aux != ';') && !(caracter_anterior != '\n'))) {
		if (aux != ':' && !read_path) {
			path[posicion_path] = aux;
			posicion_path++;
		} else if (aux == ':' && !read_path) {
			read_path = true;
		} else {
			data[posicion_data] = aux;
			posicion_data++;
		}
		caracter_anterior = aux;
		aux = fgetc(file);
	}
	data[posicion_data - 1] = '\0';
	path[posicion_path ] = '\0';
	int result = fisops_create_file(path,33204,NULL);
	if (result < 0) return result;
	result =  fisops_write_file(path, data, posicion_data - 1, 0, NULL);
	return result;
}


void *
fisops_init(struct fuse_conn_info *conn)
{
	strcpy(NAME_PERSISTENCE,"base.fisops");
	FILE *file = fopen(NAME_PERSISTENCE, "r");
	if (!file)
		return NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int i = 0;
	bool files = false;
	while ((read = getline(&line, &len, file)) != -1) {

		if (i == 0) {
			i++;
			continue;
		}
		if (line[0] == ';' || line[0] == ' ') {
			files = true;
			break;
		}
		if (!files) {
			char new_path[MAX_PATH];
			strncpy(new_path, line, read - 1);
			new_path[read - 1] = '\0';
			fisops_create_directory(new_path, __S_IFDIR | 0755);  // ver las flags
		}
	};
	int resultado = 0;
	while (resultado >= 0 && !feof(file)) {
		resultado = read_data(file);
	}
	fclose(file);
	return NULL;
}


int
fisops_change_mode(const char *path, mode_t new_mode)
{
	struct inode *inode = search_inode_by_path(path);
	if (!inode)
		return -ENOENT;
	inode->stats.st_mode = new_mode;
	return 0;
}


static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.mkdir = fisops_create_directory,
	.rmdir = fisops_remove_directory,
	.unlink = fisops_unlink,
	.create = fisops_create_file,
	.utimens = fisops_utimens,
	.write = fisops_write_file,
	.truncate = fisops_truncate,
	.rename = fisops_rename,
	.flush = fisops_flush,
	.init = fisops_init,
	.destroy = fisops_destroy,
	.chmod = fisops_change_mode,
};


void
root_initialization()
{
	struct inode *root = &super.inode_array[0];
	root->parent = NULL;
	strcpy(root->name, "/");
	root->stats.st_size = SIZE_DIRECTORY;
	root->stats.st_atim = get_time();
	root->stats.st_ctim = get_time();
	root->stats.st_mtim = get_time();
	root->stats.st_mode = __S_IFDIR | 0755;
	root->stats.st_uid = 1717;
	root->stats.st_nlink = 2;
	root->stats.st_gid = getgid();
	strcpy(root->path, "/");
	root->dir = 1;
	super.bitmap[0] = 0;
}


int
main(int argc, char *argv[])
{
	strcpy(file_path, argv[1]);
	// if (argc>2) strcpy(NAME_PERSISTENCE,argv[3]);
	// else strcpy(NAME_PERSISTENCE,"default.fisops");
	for (int i = 0; i < MAX_INODE; i++) {
		super.bitmap[i] = 1;  // todos los inodos estan disponibles
	}
	root_initialization();
	
	return fuse_main(argc, argv, &operations, NULL);
}
