
#include "dobc.hh"
#include "cover.hh"

void coverblock::set_begin(pcodeop *op) 
{
	start = op->start.getOrder();
}

void coverblock::set_end(pcodeop *op) 
{
	end = op->start.getOrder();
}

void coverblock::set_end(int e)
{
	end = e;
}

bool coverblock::contain(pcodeop *op)
{
	uintm p = op->start.getOrder();

	if (empty())
		return false;

	return (p <= end) && (p >= start);
}

int coverblock::dump(char *buf)
{
	if (end == -1)
		return sprintf(buf, "{ver:%d,ind:%d,%d-max}", version, blk_index, start);
	else
		return sprintf(buf, "{ver:%d,ind:%d,%d-%d}", version, blk_index, start, end);
}

void cover::add_def_point(varnode *vn)
{
	pcodeop *def;

	c.clear();

	def = vn->get_def();
	if (def) {
		coverblock &block(c[def->parent->index]);
		block.set_begin(def);
		block.set_end(def);
	}
	else {
		throw LowlevelError("not support input varnode");
	}
}

void cover::add_ref_point(pcodeop *ref, varnode *vn, int exclude)
{
	int i;
	flowblock *bl;

	bl = ref->parent;
	coverblock &block(c[bl->index]);
	if (block.empty()) {
		uintm last = ref->start.getOrder();
		if (exclude) {
			if ((last - 1) >= 0)
				block.set_end(last - 1);
		}
		else
			block.set_end(ref);
	}
	else if (block.contain(ref)){
		return;
	}
	else {
		block.set_end(ref);
		if (block.end >= block.start) {
			return;
		}
	}

	for (i = 0; i < bl->in.size(); i++)
		add_ref_recurse(bl->get_in(i));
}

void cover::add_ref_recurse(flowblock *bl)
{
	int i;

	coverblock &block(c[bl->index]);
	if (block.empty()) {
		block.set_all();
		for (i = 0; i < bl->in.size(); i++)
			add_ref_recurse(bl->get_in(i));
	}
	else {
		if (block.end >= block.start)
			block.set_end(INT_MAX);
	}
}

bool cover::contain(const pcodeop *op)
{
    return false;
}

int cover::dump(char *buf)
{
	int n = 0;
	map<int, coverblock>::iterator it;

	n += sprintf(buf + n, "[");
	for (it = c.begin(); it != c.end(); it++) {
		coverblock &c(it->second);
		if (c.end == INT_MAX)
			n += sprintf(buf + n, "{blk:%d, %d-max}", it->first, c.start);
		else
			n += sprintf(buf + n, "{blk:%d, %d-%d}", it->first, c.start, c.end);
	}
	n += sprintf(buf + n, "]");

	return n;
}

