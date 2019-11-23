#include "logic.h"

// globals
int COMP_DEF_COUNT = 0;
component * COMP_DEFS[MAX_COMP_DEFS];
void      * COMP_HANDLES[MAX_COMP_DEFS];

int comp_count = 0, comp_size = 0;
component* comps = NULL;

int wire_count = 0, wire_size = 0;
wire* wires = NULL;

void Logic_DefineComps() {
	DIR * d;
	struct dirent * dir;

	d = opendir(COMP_PATH);

	if (!d) return;

	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG) {
			Logic_DefineCompFile(dir->d_name);

			if (COMP_DEF_COUNT == MAX_COMP_DEFS) break;
		}
	}

	closedir(d);
}

// takes component filename (without folder prefix)
// and adds it to definitions
// returns 1 for successful load
// 0 for error
int Logic_DefineCompFile(const char * fname) {
	const int cp_len   = strlen(COMP_PATH);
	const int cpso_len = strlen(COMP_SO_PATH);
	const int fn_len   = strlen(fname);

	const int path_len = cp_len + fn_len + 2;
	char* path = malloc( sizeof(char) * path_len);

	// path = 'COMP_PATH/fname'
	strcpy(path, COMP_PATH);
	strcat(path, fname);
	printf("loading component '%s'\n", path);

	const int output_len = cpso_len + fn_len + 5;
	char* output = malloc(sizeof(char) * output_len);

	// output = 'COMP_SO_PATH/fname.so'
	strcpy(output, COMP_SO_PATH);
	strcat(output, fname);
	strcat(output, ".so");

	remove(output);

	// compiler command
	char* cc_com = malloc(sizeof(char)*
	  (path_len+output_len+80));
	strcpy(cc_com, "cc -std=c99 -O1 ");
	strcat(cc_com, path);
	strcat(cc_com, " -o ");
	strcat(cc_com, output);
	strcat(cc_com, " -shared -fPIC -include include/comp.h");

	// compile component
	//printf("%s\n", cc_com);
	system(cc_com);

	// open compiled library
	void * handle = dlopen(output, RTLD_LAZY);
	// free strings used
	free(output);
	free(path);
	free(cc_com);

	if (!handle) return 0;

	dlerror(); // clears error state

	// functions
	component* (*__load__)();
	void (*temp)();
	char * error;

	// get the __load__ function
	*(void **) (&__load__) = dlsym(handle, "__load__");
	if ((error = dlerror()) != NULL)
		return 0;

	// add definitions
	COMP_DEFS   [COMP_DEF_COUNT] = (*__load__)();
	COMP_HANDLES[COMP_DEF_COUNT] = handle;

#define __LOAD(x) if (COMP_DEFS[COMP_DEF_COUNT]->x) {\
		*(void **) (&temp)=dlsym(handle,#x);COMP_DEFS[COMP_DEF_COUNT]->x=temp;}
	//if (COMP_DEFS[COMP_DEF_COUNT].update()) 
	__LOAD(update);
	__LOAD(render);
	__LOAD(click);

	++COMP_DEF_COUNT;

	return 1;
}

// if comps is already allocated, it reallocates it
void Logic_AllocateComponents(int size) {
	if (!comps) {
		comps = malloc(size * sizeof(component));
		comp_size = size;
	} else {
		comps = realloc(comps, size * sizeof(component));
		comp_size = size;
	}
}

void Logic_AllocateWires(int size) {
	if (!wires) {
		wires = malloc(size * sizeof(wires));
		wire_size = size;
	} else {
		wires = realloc(comps, size * sizeof(wires));
		wire_size = size;
	}
}

void Logic_FreeDefines() {
	int i;
	for (i = 0; i < COMP_DEF_COUNT; ++i) {
		dlclose(COMP_HANDLES[i]);
	}
}

void Logic_FreeComponents() {
	if (comps) free(comps);
}

void Logic_FreeWires() {
	if (wires) free(wires);
}

void Logic_AddComponent(const component* comp_type, int x, int y) {
	if (comp_count == comp_size)
		Logic_AllocateComponents(comp_size*2+1);

	component * c = comps + comp_count;
	*c = *comp_type;
	c->x = x;
	c->y = y;

	++comp_count;
}

void Logic_DeleteComponent(int index) {
	if (index<0 || index>=comp_count) return;
	component * c = comps + index;

	// first, delete all wires
	// connected to the component
	int i;
	for (i = 0; i < wire_count;) {
		wire * w = wires+i;
		if (w->c1==c || w->c2==c) {
			Logic_DeleteWire(i);
		} else ++i;
	}

	for (i = index; i < comp_count-1; ++i) {
		comps[i] = comps[i+1];
	}

	--comp_count;
}

void Logic_DeleteWire(int index) {
	if (index<0 || index>=wire_count) return;
	
	int i;
	for (i = index; i < wire_count-1; ++i) {
		wires[i] = wires[i+1];
	}

	--wire_count;
}

void Logic_AddWire(component* a, component* b, int n1, int n2, int parity) {
	if (!(a&&b)) return;

	if (wire_count == wire_size)
		Logic_AllocateWires(comp_size*2+1);

	wire * w = wires + wire_count;
	w->state = 0;
	w->parity = parity;
	w->flow = 0;

	w->c1 = a;
	w->c2 = b;
	w->n1 = n1;
	w->n2 = n2;

	Component_GetNodePos(a, Component_GetNode(a,n1), &w->x1, &w->y1);
	Component_GetNodePos(b, Component_GetNode(b,n2), &w->x2, &w->y2);

	++wire_count;
}

void Logic_Update() {
	for (int i = 0; i < wire_count; ++i) {
		wire * w = wires + i;

		node * N1 = Component_GetNode(w->c1,w->n1),
		     * N2 = Component_GetNode(w->c2,w->n2);

		// if output node
		if (w->n1>=0) w->state = N1->state;
		if (w->n2>=0) w->state = N2->state;

		N1->state = w->state;
		N2->state = w->state;
	}

	Logic_UpdateComponents();
}

void Logic_UpdateComponents() {
	for (int i = 0; i < comp_count; ++i) {
		if (comps[i].update) {
			comps[i].update(comps+i);
		}
	}
}

void Logic_UpdateAllWirePos() {
	for (int i = 0; i < wire_count; ++i) {
		Logic_UpdateWirePos(wires + i);
	}
}

void Logic_UpdateWirePos(wire * w) {
	Component_GetNodePos(w->c1, Component_GetNode(w->c1,w->n1), &w->x1, &w->y1);
	Component_GetNodePos(w->c2, Component_GetNode(w->c2,w->n2), &w->x2, &w->y2);
}