#pragma once

#include "StringGraph.h"
#include "BaseVec.h"
#include <ostream>
#include <inttypes.h>

// A vertex of a directed string graph.
class DirectedStringGraphVertex : public StringGraphVertex {
public:
	size_t out_degree() const
	{
		return _edge_indices.size();
	}

	// Print a directed string graph vertex
	void print_dot(std::ostream & os, size_t v_idx) const
	{
		size_t read_idx = v_idx / 2;
		char read_dir = (v_idx & 1) ? 'E' : 'B';
		os << "v" << v_idx << " [label = \"" << (read_idx + 1)
		   << '.' << read_dir << "\"];\n";
	}
};

// An edge of a directed string graph.
class DirectedStringGraphEdge : public StringGraphEdge {
public:
	typedef unsigned int v_idx_t;
private:
	v_idx_t _v1_idx;
	v_idx_t _v2_idx;
	BaseVec _seq;

	// Serialize or deserialize the directed string graph edge to/from a
	// stream.
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive & ar, unsigned version)
	{
		ar & _v1_idx;
		ar & _v2_idx;
		ar & _seq;
	}
public:
	// Return a reference to the sequence associated with this edge of the
	// directed string graph.
	BaseVec & get_seq()
	{
		return _seq;
	}

	// Return a const reference to the sequence associated with this edge of
	// the directed string graph.
	const BaseVec & get_seq() const
	{
		return _seq;
	}

	// Return the length of the sequence associated with this edge of the
	// directed string graph.
	BaseVec::size_type length() const
	{
		return _seq.size();
	}

	// Return the index of the vertex at the tail of this edge in the
	// directed string graph.
	v_idx_t get_v1_idx() const
	{
		return _v1_idx;
	}

	// Return the index of the vertex at the head of this edge in the
	// directed string graph.
	v_idx_t get_v2_idx() const
	{
		return _v2_idx;
	}

	// Set the vertex indices at the tail (v1_idx) and head (v2_idx) of this
	// edge in the directed string graph.
	void set_v_indices(const v_idx_t v1_idx, const v_idx_t v2_idx)
	{
		_v1_idx = v1_idx;
		_v2_idx = v2_idx;
	}

	// Print this directed string graph edge.
	void print(std::ostream & os, const v_idx_t v_idx) const
	{
		v_idx_t v1_idx = get_v1_idx();
		v_idx_t read_1_idx = v1_idx / 2 + 1;
		char read_1_dir = (v1_idx & 1) ? 'E' : 'B';
		v_idx_t v2_idx = get_v2_idx();
		v_idx_t read_2_idx = v2_idx / 2 + 1;
		char read_2_dir = (v2_idx & 1) ? 'E' : 'B';
		os << read_1_idx << '.' << read_1_dir << " -> "
		   << read_2_idx << '.' << read_2_dir
		   << '\t' << get_seq();
	}

	// Print this directed string graph edge in DOT format.
	void print_dot(std::ostream & os, const v_idx_t v_idx) const
	{
		os << "v" << get_v1_idx() << " -> "
		   << "v" << get_v2_idx()
		   << " [ label = \"" << length() << "\" ];\n";
	}
};

// A directed string graph.
class DirectedStringGraph : public StringGraph<DirectedStringGraphVertex,
					       DirectedStringGraphEdge,
					       DirectedStringGraph>
{
private:
	// Add an edge to this directed string graph.
	void add_edge(const v_idx_t read_1_idx,
		      const v_idx_t read_2_idx,
		      const v_idx_t dirs,
		      const BaseVec & bv,
		      const BaseVec::size_type beg,
		      const BaseVec::size_type end)
	{
		DirectedStringGraphEdge e;
		v_idx_t v1_idx = read_1_idx * 2 + (dirs >> 1);
		v_idx_t v2_idx = read_2_idx * 2 + (dirs & 1);

		e.set_v_indices(v1_idx, v2_idx);
		bv.extract_seq(beg, end, e.get_seq());

		edge_idx_t edge_idx = this->push_back_edge(e);
		_vertices[v1_idx].add_edge_idx(edge_idx);
	}
public:
	// Initialize this directed string graph with enough space for
	// @num_reads reads to be inserted.
	DirectedStringGraph(size_t num_reads)
	{
		if (!enough_v_indices(num_reads * 2))
			fatal_error("Too many reads (%zu)", num_reads);
		_vertices.resize(num_reads * 2);
	}

	// Read this directed string graph from a file.
	DirectedStringGraph(const char *filename)
	{
		this->read(filename);
	}

	void print_dot_graph_attribs(std::ostream & os) const
	{
		os << "\tnode [shape = oval];\n";
	}

	void transitive_reduction();

	// Add a pair of edges produced by an overlap to this directed string
	// graph.
	void add_edge_pair(const v_idx_t read_1_idx,
			   const v_idx_t read_2_idx,
			   const v_idx_t dirs,
			   const BaseVec & bv1,
			   const BaseVec::size_type beg_1,
			   const BaseVec::size_type end_1,
			   const BaseVec & bv2,
			   const BaseVec::size_type beg_2,
			   const BaseVec::size_type end_2)
	{
		add_edge(read_1_idx, read_2_idx, dirs, bv1, beg_1, end_1);
		add_edge(read_2_idx, read_1_idx, (dirs ^ 3), bv2, beg_2, end_2);
	}
};
