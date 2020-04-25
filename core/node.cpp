//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "node.hpp"

#include <charconv>

#include "annotate.hpp"
#include "lgedgeiter.hpp"
#include "lgraph.hpp"

void Node::invalidate(LGraph *_g) {
  top_g     = _g;
  current_g = _g;
  hidx.invalidate();
  nid = 0;
}

void Node::invalidate() {
  current_g = top_g;
  hidx.invalidate();
  nid = 0;
}

void Node::update(const Hierarchy_index &_hidx, Index_ID _nid) {
  if (_hidx != hidx) {
    current_g = top_g->ref_htree()->ref_lgraph(_hidx);
    hidx      = _hidx;
  }
  nid = _nid;
  I(current_g->is_valid_node(nid));
}

void Node::update(const Hierarchy_index &_hidx) {
  I(_hidx != hidx);
  current_g = top_g->ref_htree()->ref_lgraph(_hidx);
  hidx      = _hidx;

  nid = current_g->fast_first();
  I(!nid.is_invalid());  // No update call if it is an empty graph

  I(current_g->is_valid_node(nid));
}

void Node::update(const Node &node) {
  top_g     = node.top_g;
  current_g = node.current_g;
  hidx      = node.hidx;
  nid       = node.nid;
}

void Node::update(LGraph *_g, const Node::Compact &comp) {
  I(comp.nid);
  I(_g);

  nid = comp.nid;
  if (top_g == nullptr) {
    top_g = _g;
    hidx  = comp.hidx;
  } else if (hidx == comp.hidx && _g == top_g)
    return;
  top_g = _g;
  hidx  = comp.hidx;
  if (hidx == Hierarchy_tree::root_index()) {
    current_g = top_g;
    return;
  }
  current_g = top_g->ref_htree()->ref_lgraph(hidx);

  I(current_g->is_valid_node(nid));
}

void Node::update(const Node::Compact &comp) {
  I(!comp.hidx.is_invalid());
  I(comp.nid);
  I(top_g);

  nid = comp.nid;
  if (hidx == comp.hidx) return;
  hidx      = comp.hidx;
  current_g = top_g->ref_htree()->ref_lgraph(hidx);

  I(current_g->is_valid_node(nid));
}

#if 0
Node::Node(LGraph *_g)
  :top_g(_g)
  ,current_g(_g)
  ,hidx(Hierarchy_tree::root_index())
  ,nid(0) {
  I(top_g);
}
#endif

Node::Node(LGraph *_g, const Hierarchy_index &_hidx, const Compact_class &comp)
    : top_g(_g), current_g(0), hidx(_hidx), nid(comp.nid) {
  I(nid);
  I(top_g);
  if (hidx.is_root())
    current_g = top_g;
  else
    current_g = top_g->ref_htree()->ref_lgraph(hidx);

  I(current_g->is_valid_node(nid));
  // I(top_g->get_hierarchy_class_lgid(hidx) == current_g->get_lgid());
}

Node::Node(LGraph *_g, const Compact_class &comp) : top_g(_g), current_g(0), hidx(Hierarchy_tree::root_index()), nid(comp.nid) {
  I(nid);
  I(top_g);

  current_g = top_g;

  I(current_g->is_valid_node(nid));
  // I(top_g->get_hierarchy_class_lgid(hidx) == current_g->get_lgid());
}

Node::Node(LGraph *_g, LGraph *_c_g, const Hierarchy_index &_hidx, Index_ID _nid)
    : top_g(_g), current_g(_c_g), hidx(_hidx), nid(_nid) {
  I(nid);
  I(top_g);
  I(current_g);
  I(current_g->is_valid_node(nid));
  // I(top_g->get_hierarchy_class_lgid(hidx) == current_g->get_lgid());
}

Node_pin Node::get_driver_pin() const {
  I(top_g->get_type(nid).has_single_output());
  return Node_pin(top_g, current_g, hidx, nid, 0, false);
}

Node_pin Node::get_sink_pin() const {
  GI(current_g, current_g->get_type(nid).has_single_input());
  return Node_pin(top_g, current_g, hidx, nid, 0, true);
}

Node_pin Node::get_driver_pin(Port_ID pid) const {
  I(current_g->get_type(nid).has_output(pid));
  Index_ID idx = current_g->find_idx_from_pid(nid, pid);
  I(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, false);
}

Node_pin Node::get_sink_pin(Port_ID pid) const {
  I(current_g->get_type(nid).has_input(pid));
  Index_ID idx = current_g->find_idx_from_pid(nid, pid);
  I(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, true);
}

Node_pin Node::get_driver_pin(std::string_view pname) const {
  auto type = get_type();
  I(current_g);  // Get type sets it

  auto pid = type.get_output_match(pname);
  I(pid != Port_invalid);  // graph_pos must be valid if connected

  auto idx = nid;
  if (pid) idx = current_g->setup_idx_from_pid(nid, pid);
  return Node_pin(top_g, current_g, hidx, idx, pid, false);
}

Node_pin Node::get_sink_pin(std::string_view pname) const {
  auto type = get_type();
  I(current_g);  // Get type sets it

  auto pid = type.get_input_match(pname);
  I(pid != Port_invalid);  // graph_pos must be valid if connected

  auto idx = nid;
  if (pid) idx = current_g->setup_idx_from_pid(nid, pid);
  return Node_pin(top_g, current_g, hidx, idx, pid, true);
}

bool Node::has_inputs() const { return current_g->has_node_inputs(nid); }

bool Node::has_outputs() const { return current_g->has_node_outputs(nid); }

int Node::get_num_inputs() const { return current_g->get_num_inputs(nid); }

int Node::get_num_outputs() const { return current_g->get_num_outputs(nid); }

Node_pin Node::setup_driver_pin(Port_ID pid) {
  I(current_g->get_type(nid).has_output(pid));
#ifndef NDEBUG
  if (current_g->is_type_sub(nid)) {
    Lg_type_id  sub_lgid = current_g->get_type_sub(nid);
    const auto &sub      = current_g->get_library().get_sub(sub_lgid);
    I(sub.has_graph_pin(pid));
    I(sub.is_output_from_graph_pos(pid), "ERROR: An input can not be a driver pin");
  }
#endif

  Index_ID idx = current_g->setup_idx_from_pid(nid, pid);
  current_g->setup_driver(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, false);
}

Node_pin Node::setup_driver_pin() const {
  I(current_g->get_type(nid).has_single_output());
  current_g->setup_driver(nid);
  return Node_pin(top_g, current_g, hidx, nid, 0, false);
}

const Node_Type &Node::get_type() const { return current_g->get_type(nid); }

void Node::set_type(const Node_Type_Op op) {
  I(op != SubGraph_Op && op != U32Const_Op && op != StrConst_Op &&
    op != LUT_Op);  // do not set type directly, call set_type_const_value ....
  current_g->set_type(nid, op);
}

void Node::set_type(const Node_Type_Op op, uint32_t bits) {
  current_g->set_type(nid, op);

  setup_driver_pin().set_bits(bits);  // bits only possible when the cell has a single output
}

bool Node::is_type(const Node_Type_Op op) const { return current_g->get_type(nid).op == op; }

bool Node::is_type_io() const { return current_g->is_graph_io(nid); }

bool Node::is_type_loop_breaker() const { return current_g->is_type_loop_breaker(nid); }

bool Node::is_type_sub() const { return current_g->is_sub(nid); }

bool Node::is_type_const() const { return current_g->is_type_const(nid); }

Hierarchy_index Node::hierarchy_go_down() const {
  I(current_g->is_sub(nid));
  return top_g->ref_htree()->go_down(*this);
}

Hierarchy_index Node::hierarchy_go_up() const {
  I(current_g != top_g);
  return top_g->ref_htree()->go_up(*this);
}

bool Node::is_root() const {
  bool ans = top_g == current_g;
  I(top_g->ref_htree()->is_root(*this) == ans);
  return ans;
}

Node Node::get_up_node() const {
  I(!is_root());
  auto up_node = top_g->ref_htree()->get_instance_up_node(hidx);

  return up_node;
}

void Node::set_type_sub(Lg_type_id subid) { current_g->set_type_sub(nid, subid); }

Lg_type_id Node::get_type_sub() const { return current_g->get_type_sub(nid); }

LGraph *Node::ref_type_sub_lgraph() const {
  auto lgid = current_g->get_type_sub(nid);
  return LGraph::open(top_g->get_path(), lgid);
}

bool Node::is_type_sub_present() const {
  if (!current_g->is_type_sub(nid)) return false;

  auto *sub_lg = ref_type_sub_lgraph();
  if (sub_lg) return !sub_lg->is_empty();

  return false;
}

void Node::set_type_lut(Lut_type_id lutid) { current_g->set_type_lut(nid, lutid); }

Lut_type_id Node::get_type_lut() const { return current_g->get_type_lut(nid); }

const Sub_node &Node::get_type_sub_node() const { return current_g->get_type_sub_node(nid); }

Sub_node *Node::ref_type_sub_node() const { return current_g->ref_type_sub_node(nid); }

/* DEPRECATED
void Node::set_type_const_value(std::string_view str) {
  g->set_type_const_value(nid, str);
}

void Node::set_type_const_sview(std::string_view str) {
  g->set_type_const_sview(nid, str);
}

void Node::set_type_const_value(uint32_t val) {
  g->set_type_const_value(nid, val);
}
*/

uint32_t Node::get_type_const_value() const { return current_g->get_type_const_value(nid); }

std::string_view Node::get_type_const_sview() const { return current_g->get_type_const_sview(nid); }

Node_pin Node::setup_driver_pin(std::string_view name) {
  auto type = get_type();
  I(current_g);  // Get type sets it

  auto pid = type.get_output_match(name);
  if (pid != Port_invalid) {
    auto idx = nid;
    if (pid) idx = current_g->setup_idx_from_pid(nid, pid);
    current_g->setup_driver(idx);
    return Node_pin(top_g, current_g, hidx, idx, pid, false);
  }

  I(type.op == SubGraph_Op);

  Lg_type_id sub_lgid = current_g->get_type_sub(nid);
  I(current_g->get_library().exists(sub_lgid));  // Must be a valid lgid

  const auto &sub = current_g->get_library().get_sub(sub_lgid);
  I(sub.has_pin(name));
  I(sub.is_output(name));

  pid = sub.get_graph_pos(name);
  I(pid != Port_invalid);  // graph_pos must be valid if connected

  Index_ID idx = current_g->setup_idx_from_pid(nid, pid);
  current_g->setup_driver(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, false);
}

void Node::nuke() {
  I(false);  // TODO:
}

Node_pin Node::setup_sink_pin(std::string_view name) {
  auto type = get_type();
  I(current_g);  // Get type sets it

  if (type.op != SubGraph_Op) {
    auto pid = type.get_input_match(name);
    if (pid != Port_invalid) {
      auto idx = nid;
      if (pid) idx = current_g->setup_idx_from_pid(nid, pid);
      current_g->setup_sink(idx);
      return Node_pin(top_g, current_g, hidx, idx, pid, true);
    }
    return Node_pin(top_g, current_g, hidx, 0, Port_invalid, true);
  }

  I(type.op == SubGraph_Op);

  Lg_type_id sub_lgid = current_g->get_type_sub(nid);
  I(current_g->get_library().exists(sub_lgid));  // Must be a valid lgid

  const auto &sub = current_g->get_library().get_sub(sub_lgid);
  I(sub.has_pin(name));
  I(sub.is_input(name));

  auto pid = sub.get_graph_pos(name);
  I(pid != Port_invalid);  // graph_pos must be valid if connected

  Index_ID idx = current_g->setup_idx_from_pid(nid, pid);
  current_g->setup_sink(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, true);
}

Node_pin Node::setup_sink_pin(Port_ID pid) {
  I(current_g->get_type(nid).has_input(pid));
#ifndef NDEBUG
  if (current_g->is_type_sub(nid)) {
    Lg_type_id  sub_lgid = current_g->get_type_sub(nid);
    const auto &sub      = current_g->get_library().get_sub(sub_lgid);
    I(sub.has_graph_pin(pid));
  }
#endif

  Index_ID idx = current_g->setup_idx_from_pid(nid, pid);
  current_g->setup_sink(idx);
  return Node_pin(top_g, current_g, hidx, idx, pid, true);
}

Node_pin Node::setup_sink_pin() const {
  I(current_g->get_type(nid).has_single_input());
  current_g->setup_sink(nid);
  return Node_pin(top_g, current_g, hidx, nid, 0, true);
}

XEdge_iterator Node::inp_edges() const { return current_g->inp_edges(*this); }

XEdge_iterator Node::out_edges() const { return current_g->out_edges(*this); }

XEdge_iterator Node::inp_edges_ordered() const { return current_g->inp_edges_ordered(*this); }

XEdge_iterator Node::out_edges_ordered() const { return current_g->out_edges_ordered(*this); }

Node_pin_iterator Node::inp_connected_pins() const { return current_g->inp_connected_pins(*this); }

Node_pin_iterator Node::out_connected_pins() const { return current_g->out_connected_pins(*this); }

Node_pin_iterator Node::inp_setup_pins() const { return current_g->inp_setup_pins(*this); }
Node_pin_iterator Node::out_setup_pins() const { return current_g->out_setup_pins(*this); }

void Node::del_node() { current_g->del_node(nid); }

void Node::set_name(std::string_view iname) { Ann_node_name::ref(current_g)->set(get_compact_class(), iname); }

std::string_view Node::create_name() const {
  auto *     ref = Ann_node_name::ref(current_g);
  const auto it  = ref->find(get_compact_class());
  if (it != ref->end()) return ref->get_val_sview(it);

  std::string sig = absl::StrCat("lg_", get_type().get_name(), std::to_string(nid));
  const auto  it2 = ref->set(get_compact_class(), sig);
  return ref->get_val_sview(it2);
#if 0
  // FIXME: HERE. Does not scale for large designs (too much recursion)

  if (get_type().op == GraphIO_Op) {
    absl::StrAppend(&signature, "_io");
	}else if (get_type().op == SubGraph_Op) {
    absl::StrAppend(&signature, "_", get_type_sub_node().get_name());
  }

  for(const auto &e:inp_edges()) {
    absl::StrAppend(&signature, "_", e.driver.create_name());
  }

  auto nod = Ann_node_name::find(current_g, signature);
  if (nod.is_invalid()) {
    return Ann_node_name::set(*this, signature);
  }

  absl::StrAppend(&signature,"_", std::to_string(nid)); // OK, add to stop trying

  I(Ann_node_name::find(current_g, signature).is_invalid());

  return Ann_node_name::set(*this, signature);
#endif
}

std::string_view Node::get_name() const { return Ann_node_name::ref(current_g)->get_val_sview(get_compact_class()); }

std::string Node::debug_name() const {
#ifndef NDEBUG
  static uint16_t conta = 8192;
  if (conta++ == 0) {
    fmt::print("WARNING: Node::debug_name should not be called during release (Slowww!)\n");
  }
#endif
  auto *      ref = Ann_node_name::ref(current_g);
  std::string name;
  const auto  it = ref->find(get_compact_class());
  if (it != ref->end()) {
    name = ref->get_val_sview(it);
  }
  if (current_g->is_type_sub(nid)) {
    absl::StrAppend(&name, "_sub_", get_type_sub_node().get_name());
  }

  if (name.empty()) return absl::StrCat("node_", std::to_string(nid), "_", get_type().get_name(), "_lg_", current_g->get_name());
  return absl::StrCat("node_", std::to_string(nid), "_", get_type().get_name(), "_", name, "_lg_", current_g->get_name());
}

bool Node::has_name() const { return Ann_node_name::ref(current_g)->has_key(get_compact_class()); }

const Ann_place &Node::get_place() const {
  auto *data = Ann_node_place::ref(top_g)->ref(get_compact());
  I(data);
  return *data;
}

Ann_place *Node::ref_place() {
  auto *ref = Ann_node_place::ref(top_g);

  auto it = ref->find(get_compact());
  if (it != ref->end()) {
    return ref->ref(it);
  }

  auto it2 = ref->set(get_compact(), Ann_place());  // Empty
  return ref->ref(it2);
}

bool Node::has_place() const { return Ann_node_place::ref(top_g)->has(get_compact()); }


//----- Subject to changes in the future:
#define WHITE 0
#define GREY  1
#define BLACK 2
void Node::set_color(int new_color) {
  Ann_node_color::ref(current_g)->set(get_compact_class(), std::to_string(new_color));
}

int Node::get_color() const {
  auto str = Ann_node_color::ref(current_g)->get_val_sview(get_compact_class());
  int color;
  std::from_chars(str.data(), str.data() + str.size(), color);
  return color;
}

bool Node::has_color() const {
  return Ann_node_color::ref(current_g)->has_key(get_compact_class());
}
