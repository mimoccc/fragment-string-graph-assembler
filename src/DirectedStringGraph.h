#pragma once

#include "StringGraph.h"
#include "BaseVec.h"
#include <ostream>
#include <inttypes.h>
#include <boost/serialization/base_object.hpp>

class BidirectedStringGraph;

// A vertex of a directed string graph.
class DirectedStringGraphVertex : public StringGraphVertex {
private:
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive & ar, unsigned version)
	{
		ar & boost::serialization::base_object<StringGraphVertex>(*this);
	}
public:
	size_t out_degree() const
	{
		return _edge_indices.size();
	}

	// Print a directed string graph vertex in DOT format.
	void print_dot(std::ostream & os, size_t v_idx) const
	{
		size_t read_idx = v_idx / 2;
		char read_dir = (v_idx & 1) ? 'E' : 'B';
		os << "\tv" << v_idx << " [ label=\""
		   << (read_idx + 1) << '.' << read_dir << '\"'
		   << " fillcolor=" << (read_dir == 'E' ? "green" : "cyan")
		   << " ];\n";
	}
};

// An edge of a directed string graph.
class DirectedStringGraphEdge : public StringGraphEdge {
public:
	typedef unsigned int v_idx_t;
private:
	v_idx_t  _v1_idx;
	v_idx_t  _v2_idx;
	BaseVec  _seq;

	// Serialize or deserialize the directed string graph edge to/from a
	// stream.
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive & ar, unsigned version)
	{
		ar & boost::serialization::base_object<StringGraphEdge>(*this);
		ar & _v1_idx;
		ar & _v2_idx;
		ar & _seq;
	}
public:
	// Return a reference to the sequence associated with this edge of the
	// directed string graph.
	BaseVec & get_seq() { return _seq; }
	const BaseVec & get_seq() const { return _seq; }

	// Return the length of the sequence associated with this edge of the
	// directed string graph.
	BaseVec::size_type length() const { return _seq.size(); }

	// Return the index of the vertex at the tail of this edge in the
	// directed string graph.
	v_idx_t get_v1_idx() const { return _v1_idx; }

	// Return the index of the vertex at the head of this edge in the
	// directed string graph.
	v_idx_t get_v2_idx() const { return _v2_idx; }

	// Get the vertex indices at the tail (v1_idx) and head (v2_idx) of this
	// edge in the directed string graph.
	void get_v_indices(v_idx_t & v1_idx, v_idx_t & v2_idx) const
	{
		v1_idx = _v1_idx;
		v2_idx = _v2_idx;
	}

	// Set the vertex indices at the tail (v1_idx) and head (v2_idx) of this
	// edge in the directed string graph.
	void set_v_indices(const v_idx_t v1_idx, const v_idx_t v2_idx)
	{
		_v1_idx = v1_idx;
		_v2_idx = v2_idx;
	}

	void set_v1_idx(const v_idx_t v1_idx) { _v1_idx = v1_idx; }

	void set_v2_idx(const v_idx_t v2_idx) { _v2_idx = v2_idx; }

	// Print this directed string graph edge.
	void print(std::ostream & os = std::cout, const v_idx_t v_idx = 0,
		   const bool print_seqs = true) const
	{
		const v_idx_t read_1_idx = get_v1_idx() / 2 + 1;
		const v_idx_t read_2_idx = get_v2_idx() / 2 + 1;

		const char read_1_dir = (get_v1_idx() & 1) ? 'E' : 'B';
		const char read_2_dir = (get_v2_idx() & 1) ? 'E' : 'B';

		os << read_1_idx << '.' << read_1_dir << " -> "
		   << read_2_idx << '.' << read_2_dir
		   << '\t';
		StringGraphEdge::print(os);
		if (print_seqs)
			os << _seq;
		else
			os << _seq.length();
	}

	friend std::ostream & operator<<(std::ostream & os,
					 const DirectedStringGraphEdge & e)
	{
		e.print(os);
		return os;
	}

	// Print this directed string graph edge in DOT format.
	void print_dot(std::ostream & os, const v_idx_t v_idx,
		       const bool print_seqs) const
	{
		os << "\tv" << get_v1_idx() << " -> "
		   << "v" << get_v2_idx()
		   << " [ label=\"";
		if (print_seqs)
			os << _seq;
		else
			os << _seq.length();
		os << "\" ";
		if (_traversal_count != 0) {
			os << "color=red ";
		}
		os << "];\n";
	}
};

// A directed string graph.
class DirectedStringGraph : public StringGraph<DirectedStringGraphVertex,
					       DirectedStringGraphEdge,
					       DirectedStringGraph>
{
private:
	std::vector<std::vector<edge_idx_t> > _back_edges;

	// Add an edge to this directed string graph.
	void add_edge(const v_idx_t v1_idx,
		      const v_idx_t v2_idx,
		      const BaseVec & bv,
		      const BaseVec::size_type beg,
		      const BaseVec::size_type end,
		      const bool rc)
	{
		DirectedStringGraphEdge e;

		e.set_v_indices(v1_idx, v2_idx);
		bv.extract_seq(beg, end, rc, e.get_seq());

		edge_idx_t edge_idx = this->push_back_edge(e);
		_vertices[v1_idx].add_edge_idx(edge_idx);
	}

	DirectedStringGraphEdge &
	add_unlabeled_edge(const v_idx_t v1_idx,
			   const v_idx_t v2_idx)
	{
		DirectedStringGraphEdge e;
		e.set_v_indices(v1_idx, v2_idx);
		edge_idx_t edge_idx = this->push_back_edge(e);
		_vertices[v1_idx].add_edge_idx(edge_idx);
		return _edges[edge_idx];
	}

				
public:
	// Initialize this directed string graph with enough space for
	// @num_reads reads to be inserted.
	DirectedStringGraph(size_t num_reads)
	{
		if (!enough_v_indices(num_reads * 2))
			fatal_error("Too many reads (%zu)", num_reads);
		_vertices.resize(num_reads * 2);
		_orig_num_reads = num_reads;
	}

	// Read this directed string graph from a file.
	DirectedStringGraph(const char *filename)
	{
		this->read(filename);
	}

	static const char magic[10];

	void print_dot_graph_attribs(std::ostream & os) const
	{
		//os << "\tnode [shape = oval];\n";
		os << "\tnode [style=filled];\n";
	}

	void transitive_reduction();
	void min_cost_circulation();
	void collapse_unbranched_paths();
	void calculate_A_statistics();
	void print_stats(std::ostream & os) const;

	void map_contained_read(const v_idx_t downstream_read_idx,
				const v_idx_t downstream_read_dir,
				const BaseVec::size_type overhang_len);

	void build_from_bidigraph(const BidirectedStringGraph & bidigraph);

	// Add a pair of edges produced by an overlap to this directed string
	// graph.
	void add_edge_pair(const v_idx_t read_1_idx,
			   const v_idx_t read_2_idx,
			   const v_idx_t dirs,
			   const BaseVec & bv1,
			   const BaseVec::size_type beg_1,
			   const BaseVec::size_type end_1,
			   const bool bv1_rc,
			   const BaseVec & bv2,
			   const BaseVec::size_type beg_2,
			   const BaseVec::size_type end_2,
			   const bool bv2_rc)
	{
		const v_idx_t v1_idx = read_1_idx * 2;
		const v_idx_t v2_idx = read_2_idx * 2;
		const v_idx_t f_dir = dirs >> 1;
		const v_idx_t g_dir = dirs & 1;

		assert((dirs & 3) == dirs);

		add_edge(v1_idx ^ f_dir, v2_idx ^ g_dir,
			 bv1, beg_1, end_1, bv1_rc);

		add_edge(v2_idx ^ g_dir ^ 1, v1_idx ^ f_dir ^ 1,
			 bv2, beg_2, end_2, bv2_rc);
	}

private:
	void follow_unbranched_path(DirectedStringGraphEdge & e,
				    std::vector<bool> & remove_edge,
				    std::vector<bool> & remove_vertex,
				    const std::vector<bool> & v_inner);
	void mark_component(const v_idx_t v_idx,
			    std::vector<bool> & visited,
			    v_idx_t & component_size) const;

	void walk_back_edges(const v_idx_t v_idx,
			     size_t overhang_len,
			     edge_idx_t mapped_edges[],
			     size_t & num_mapped_edges,
			     size_t max_mapped_edges);

public:
	void assert_graph_valid() const
	{
		for (size_t i = 0; i < num_edges(); i++) {
			const DirectedStringGraphEdge & e = _edges[i];
			assert(e.get_v1_idx() < num_vertices());
			assert(e.get_v2_idx() < num_vertices());
			edge_idx_t j = locate_edge(e.get_v2_idx() ^ 1, e.get_v1_idx() ^ 1);
			assert(j < num_edges());
			const DirectedStringGraphEdge & e_X = _edges[j];
			assert((e_X.get_v1_idx() ^ 1) == e.get_v2_idx());
			assert((e_X.get_v2_idx() ^ 1) == e.get_v1_idx());
			//assert(e_X.get_num_inner_vertices() == e.get_num_inner_vertices());
		}
	}

	void extract_edge_seqs(BaseVecVec & bvv)
	{
		foreach (DirectedStringGraphEdge & e, _edges) {
			bvv.push_back(e.get_seq());
		}
	}
};
