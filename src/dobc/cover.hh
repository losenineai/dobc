
#ifndef __cover_hh__
#define __cover_hh__

#include "types.h"
#include <limits.h>
#include <map>

using namespace std;

class pcodeop;
class varnode;
class flowblock;

class coverblock {
public:
	short	version;
	short	blk_index;
	/* 这个结构主要参考自Ghidra的CoverBlock，之所以start和end，没有采用pcodeop结构是因为
	我们在优化时，会删除大量的pcodeop，这个pcode很容易失效 */
	int		start = -1;
	int		end = -1;

	coverblock() {}
	~coverblock() {}

	void set_begin(pcodeop *op);
	void set_end(pcodeop *op);
	void set_end(int);
	bool empty() {
		return (start == -1) && (end == -1);
	}

	bool contain(pcodeop *op);
	void set_all() {
		start = 0;
		end = INT_MAX;
	}
	int dump(char *buf);
};

class cover {
public:
	map<int, coverblock> c;

	void clear() { c.clear(); }
	void add_def_point(varnode *vn);
	void add_ref_point(pcodeop *op, varnode *vn, int exclude);
	void add_ref_recurse(flowblock *bl);
    bool contain(const pcodeop *op);
	int dump(char *buf);
};

#endif