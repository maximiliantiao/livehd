//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <charconv>

#include "likely.hpp"
#include "lnast_parser.hpp"

Lnast_parser::Lnast_parser():top_module_name("INVALID"), line_num(0), line_tkcnt(1) {
  ;
}

Lnast_parser::Lnast_parser(std::string_view f): top_module_name(get_module_name(f)), line_num(0), line_tkcnt(1) {
  parse_file(f);
}

Lnast_parser::Lnast_parser(std::string_view _top_module_name, std::string_view _text): top_module_name(_top_module_name), line_num(0), line_tkcnt(1) {
  parse_inline(_text);
}

std::string Lnast_parser::get_module_name(std::string_view filename) {
  std::vector<std::string> filepath_split = absl::StrSplit(filename, '/');
  std::pair<std::string, std::string> fname = absl::StrSplit(filepath_split[filepath_split.size() - 1], '.');
  return fname.first;
}

void Lnast_parser::elaborate() {
  lnast = std::make_unique<Lnast>(top_module_name, transfer_memblock_ownership());

  lnast->set_root(Lnast_node(Lnast_ntype::create_top(), Token()));

  process_stmts_op(lnast->get_root(), 1);
  build_lnast();

  //for(const auto &index : lnast.depth_preorder()) {
  //  lnast.get_data(index).dump();
  //}
}

void Lnast_parser::build_lnast() {
  uint32_t    cfg_nidx    = 0;
  uint32_t    cfg_nparent = 0;
  Token       cfg_token_beg;
  Token       cfg_token_end;
  Token       loc;
  Token       target_name;
  Lnast_ntype type;
  Lnast_nid opr_parent_node; //opr means operator
  buffer_tmp_func_def_name_idx = Lnast_nid(-1, -1); //FIXME->sh: don't want to initialize through constructor...
  buffer_tmp_funcall_idx       = Lnast_nid(-1, -1);
  line_tkcnt = 1;
  while (line_num == scan_get_token().line) {
    if(unlikely(scan_is_end()))
      return;

    I(line_tkcnt == CFG_IDX_POS);
    auto t = scan_text();
    fmt::print("pos1 scan_text:{}\n", t);
    std::from_chars(t.data(), t.data() + t.size(), cfg_nidx);
    walk_next_token();


    I(line_tkcnt == CFG_PARENT_POS);
    auto t2 = scan_text();
    std::from_chars(t2.data(), t2.data()+t.size(), cfg_nparent);
    walk_next_token();
    opr_parent_node = cfg_parent_id2lnast_node[cfg_nparent];


    I(line_tkcnt == CFG_CHILD_POS);
    walk_next_token();

    I(line_tkcnt == CFG_TOKEN_POS_BEG);
    if (unlikely(scan_text().substr(0,4) == "SEQ0")) {
      walk_next_line();
      continue;
    } else if (unlikely(scan_text().substr(0,3) == "SEQ")) {
      process_stmts_op(opr_parent_node, cfg_nidx);
      walk_next_line();
      continue;
    } else if (scan_text().substr(0,3) == "TUP") {
      process_tuple_op(opr_parent_node, cfg_nidx);
      walk_next_line();
      continue;
    } else {
      cfg_token_beg = scan_get_token();
      walk_next_token();
    }

    I(line_tkcnt == CFG_TOKEN_POS_END);
    cfg_token_end = scan_get_token();
    walk_next_token();


    I(line_tkcnt == CFG_OP_POS);
    //no regular pattern, might walk_next_token() internally case by case
    type = operator_analysis();
    walk_next_token(); // go to the target_name of operator


    I(token_is_valid_ref());
    target_name = scan_get_token();

    if (type.is_assign() && unlikely(function_def_name_correction(type, target_name))) {
      walk_next_token();
      walk_next_line();
      continue;
    } else if (type.is_assign() && unlikely(function_instance_name_correction(type, target_name))) {
      walk_next_token();
      walk_next_line();
      continue;
    } else if (type.is_assign() && unlikely(tuple_name_correction(type, target_name))) {
      walk_next_token();
      walk_next_line();
      continue;
    }

    auto tree_idx_opr = process_operator_node(opr_parent_node, type, cfg_nidx, target_name);

    //don't need to build subtree at the cfg line of if ___k condition
    if(unlikely(tree_idx_opr.is_invalid())){
      walk_next_line();
      continue;
    }

    walk_next_token(); //go to 1st operand

    I(!tree_idx_opr.is_invalid());
    add_operator_subtree(tree_idx_opr, target_name);
    walk_next_line();
  }
}

void Lnast_parser::process_tuple_op(const Lnast_nid& parent_of_tup, uint32_t self_idx) {
  auto idx_tuple_seq = lnast->add_child(parent_of_tup, Lnast_node::create_tuple(scan_get_token()));
  cfg_parent_id2lnast_node[self_idx] = idx_tuple_seq;
  fmt::print("tuple sequence :{}\n", scan_get_token().get_text());
  walk_next_token();
  fmt::print("tuple sequence name :{}\n", scan_get_token().get_text());
  buffer_tmp_tuple_name_idx = lnast->add_child(idx_tuple_seq, Lnast_node::create_ref(scan_get_token())); //tuple_name
}

void Lnast_parser::process_stmts_op(const Lnast_nid& parent_of_sts, uint32_t self_idx) {
  if (lnast->get_data(parent_of_sts).type.is_top()) {
    auto tree_top_sts = lnast->add_child(parent_of_sts, Lnast_node::create_stmts(scan_get_token(3)));
    fmt::print("stmts name :{}\n", scan_get_token().get_text());
    cfg_parent_id2lnast_node[self_idx] = tree_top_sts;
  } else if (lnast->get_data(parent_of_sts).type.is_if()) {
    if (!buffer_if_condition_used) {
      //no need to create SSA table for csts
      auto if_csts = lnast->add_child(parent_of_sts, Lnast_node::create_cstmts(scan_get_token()));
      fmt::print("stmts name :{}\n", scan_get_token().get_text());
      cfg_parent_id2lnast_node[self_idx] = if_csts;
      lnast->add_child(parent_of_sts, Lnast_node::create_cond(buffer_if_condition));
      buffer_if_condition_used = true;
    } else { // normal stmts
      auto if_sts = lnast->add_child(parent_of_sts, Lnast_node::create_stmts(scan_get_token()));
      fmt::print("stmts name :{}\n", scan_get_token().get_text());
      cfg_parent_id2lnast_node[self_idx] = if_sts;
    }
  } else if (lnast->get_data(parent_of_sts).type.is_func_def()) {
    auto tree_func_def_sts = lnast->add_child(parent_of_sts, Lnast_node::create_stmts(scan_get_token()));
    fmt::print("stmts name :{}\n", scan_get_token().get_text());
    cfg_parent_id2lnast_node[self_idx] = tree_func_def_sts;
  }
}

//scan pos start from the end of operator token
Lnast_nid Lnast_parser::process_operator_node(const Lnast_nid& opr_parent_node, Lnast_ntype type, uint32_t self_idx, const Token& target_name) {
  if (type.is_func_def()) {
    auto func_def_root = lnast->add_child(opr_parent_node, Lnast_node::create_func_def(Token()));
    cfg_parent_id2lnast_node[self_idx] = func_def_root;
    return func_def_root;
  } else if (type.is_if()) {
    auto if_idx = lnast->add_child(opr_parent_node, Lnast_node::create_if(Token()));
    cfg_parent_id2lnast_node[self_idx] = if_idx;
    buffer_if_condition = target_name;
    buffer_if_condition_used = false;
    return Lnast_nid(-1, -1);
  } else if (type.is_elif()) {
    buffer_if_condition = target_name;
    buffer_if_condition_used = false;
    return Lnast_nid(-1, -1);
  } else if (type.is_func_call()) {
    auto func_call_idx = lnast->add_child(opr_parent_node, Lnast_node::create_func_call(Token()));
    buffer_tmp_funcall_idx = func_call_idx;
    return func_call_idx;
  }

  return lnast->add_child(opr_parent_node, Lnast_node(type, Token()));
}

//scan pos start: first operand token, stop: last operand
void Lnast_parser::add_operator_subtree(const Lnast_nid& tree_idx_opr, const Token& target_name) {
  //fmt::print("token is :{}\n", scan_text());
  const auto nt = lnast->get_data(tree_idx_opr).type;

  if (nt.is_assign() || nt.is_dp_assign() || nt.is_as() || nt.is_tuple()) {//sh:fixme: handle tuple seperately
    process_assign_like_op(tree_idx_opr, target_name);
  } else if (nt.is_func_call()) {
    process_func_call_op(tree_idx_opr, target_name);
  } else if (nt.is_func_def()) {
    process_func_def_op(tree_idx_opr, target_name);
  } else {
    process_binary_op(tree_idx_opr, target_name);
  }
}


//scan pos start: first operand token, stop: last operand
void  Lnast_parser::process_func_def_op(const Lnast_nid& tree_idx_fdef, const Token& target_name) {
  //10  1  8  59  96  ::{  ___e   null $a    $b  %o

  buffer_tmp_func_def_name_idx = lnast->add_child(tree_idx_fdef, Lnast_node::create_ref(target_name));

  if (scan_text() != "null") { // case of conditional function definition
    lnast->add_child(tree_idx_fdef, Lnast_node::create_cond(scan_get_token())); //conditional function def
  } else {
    lnast->add_child(tree_idx_fdef, Lnast_node::create_cond("true", 0, 0, 0));
  }
  walk_next_token();

  auto local_line_num = scan_line();
  while (scan_line() == local_line_num) {
    I(token_is_valid_ref());
    lnast->add_child(tree_idx_fdef, Lnast_node(operand_analysis(), scan_get_token()));
    walk_next_token(); //go to $a -> go to $b -> ...
  }
  scan_prev(); //for the final dummy scan_next() in while loop
}


//scan pos start: first operand token, stop: last operand
void  Lnast_parser::process_func_call_op(const Lnast_nid& tree_idx_fcall, const Token& target_name) {
  //true function call case: K17  K18  0  98  121  .()  ___g  fun1  ___h   ___i
  //fake function call case: K19  K20  0  123 129  .()  ___j  $a

  lnast->add_child(tree_idx_fcall, Lnast_node::create_ref(target_name));

  I(token_is_valid_ref());
  lnast->add_child(tree_idx_fcall, Lnast_node(operand_analysis(), scan_get_token()));
  walk_next_token(); //go to ___h

  if(scan_line() == line_num + 1){
    //FIXME->sh: only one operand, fake function call for now!!!
    //lnast->ref_data(tree_idx_fcall)->type = Lnast_ntype::create_assign();
    scan_prev();
    return;
  }

  auto local_line_num = scan_line();
  while (scan_line() == local_line_num && !scan_is_end()) {
    I(token_is_valid_ref());
    lnast->add_child(tree_idx_fcall, Lnast_node(operand_analysis(), scan_get_token()));
    walk_next_token(); //go to ___i -> ___j ...
  }
  scan_prev(); //for the final dummy scan_next() in while loop
}

//scan pos start: first operand token, stop: last operand
void Lnast_parser::process_binary_op(const Lnast_nid& tree_idx_opr, const Token& target_name) {
  lnast->add_child(tree_idx_opr, Lnast_node::create_ref(target_name));
  I(token_is_valid_ref());
  lnast->add_child(tree_idx_opr, Lnast_node(operand_analysis(), scan_get_token()));
  walk_next_token(); //go to 2nd operand
  I(scan_is_token(Token_id_alnum) || scan_is_token(Token_id_output) || scan_is_token(Token_id_input));
  lnast->add_child(tree_idx_opr, Lnast_node(operand_analysis(), scan_get_token()));
}


//scan pos start: first operand token, stop: last operand
void Lnast_parser::process_assign_like_op(const Lnast_nid& tree_idx_opr, const Token& target_name) {
  lnast->add_child(tree_idx_opr, Lnast_node::create_ref(target_name));
  I(scan_is_token(Token_id_alnum) || scan_is_token(Token_id_output) || scan_is_token(Token_id_input) || scan_is_token(Token_id_reference));
  lnast->add_child(tree_idx_opr, Lnast_node(operand_analysis(), scan_get_token()));
}

Lnast_ntype  Lnast_parser::operand_analysis() {
  if (scan_text().at(0) == '0' || scan_text().at(0) == '-') {
    return Lnast_ntype::create_const();
  } else {
    return Lnast_ntype::create_ref();//includes io and reg such as $a, %b, @r
  }
}


bool Lnast_parser::tuple_name_correction(Lnast_ntype type, const Token &target_name) {
  I(type.is_assign());
  if (buffer_tmp_tuple_name_idx.level != -1 &&
      lnast->get_name(buffer_tmp_tuple_name_idx) == scan_peep_text(1)) {
    lnast->ref_data(buffer_tmp_tuple_name_idx)->token = target_name;
    return true;
  }
  return false;
}

bool Lnast_parser::function_def_name_correction(Lnast_ntype type, const Token& target_name) {
  I(type.is_assign());
  if (scan_peep_is_token(Token_id_reference, 1) && scan_peep_text(1).substr(1,3) == "___") {
    lnast->ref_data(buffer_tmp_func_def_name_idx)->token = target_name;
    return true;
  }
  return false;
}


bool Lnast_parser::function_instance_name_correction(Lnast_ntype type, const Token& target_name) {
  I(type.is_assign());
  if(buffer_tmp_funcall_idx == Lnast_nid(-1, -1))
    return false;

  if (scan_peep_text(1).substr(0,3) == "___") {
    auto instance_name_idx        = lnast->get_first_child(buffer_tmp_funcall_idx);
    auto function_def_name_idx    = lnast->get_sibling_next(instance_name_idx);
    auto function_def_name        = lnast->get_data(function_def_name_idx).token.get_text();
    std::string  buffer_function_def_name;
    if(buffer_tmp_func_def_name_idx != Lnast_nid(-1,-1)){
      buffer_function_def_name = lnast->get_data(buffer_tmp_func_def_name_idx).token.get_text();
    } else {
      buffer_function_def_name = "";
    }

    //handle fake function call
    if (function_def_name == buffer_function_def_name) {
      //true function call
      lnast->ref_data(instance_name_idx)->token = target_name;
      buffer_tmp_funcall_idx = Lnast_nid(-1,-1);
      return true;
    } else {
      //fake function call
      lnast->ref_data(buffer_tmp_funcall_idx)->type = Lnast_ntype::create_assign();
      lnast->ref_data(instance_name_idx)->token = target_name;
      buffer_tmp_funcall_idx = Lnast_nid(-1,-1);
      return true;
    }

    if(lnast->get_data(instance_name_idx).token.get_text() == scan_peep_text(1)){
      lnast->ref_data(instance_name_idx)->token = target_name;
      buffer_tmp_funcall_idx = Lnast_nid(-1,-1);
      return true;
    }
  }
  return false;
}


//scan pos will stop at the end of operator token
Lnast_ntype Lnast_parser::operator_analysis() {
  Lnast_ntype type;
  if (scan_is_token(Token_id_op)) { //deal with ()
    type = Lnast_ntype::create_tuple(); // must be a tuple op
    I(scan_peep_is_token(Token_id_cp, 1));
    walk_next_token();
  } else if (scan_is_token(Token_id_colon)) {
      if (scan_peep_is_token(Token_id_colon, 1)) { //handle ::{
        type = Lnast_ntype::create_func_def();
        walk_next_token();
        I(scan_peep_is_token(Token_id_ob, 1)); //must be a function def op
        walk_next_token();
      } else if (scan_peep_is_token(Token_id_eq, 1)) { //handle :=
        type = Lnast_ntype::create_dp_assign();
        walk_next_token();
      } else {
        type = Lnast_ntype::create_label();
      }
  } else if (scan_is_token(Token_id_gt)) {
    if (scan_peep_is_token(Token_id_gt, 1) and scan_peep_is_token(Token_id_gt, 2)) {
      type = Lnast_ntype::create_arith_shift_right();
      walk_next_token();
      walk_next_token();
    } else if (scan_peep_is_token(Token_id_gt, 1)) {
      // FIXME->sh: how to differentiate shift right and logic shift right?
      type = Lnast_ntype::create_shift_right();
      walk_next_token();
    } else {
      type = Lnast_ntype::create_gt();
    }
  } else if (scan_is_token(Token_id_lt)) {
    if (scan_peep_is_token(Token_id_lt, 1)) {
      type = Lnast_ntype::create_shift_left();
      walk_next_token();
    } else {
      type = Lnast_ntype::create_lt();
    }
  } else if (scan_is_token(Token_id_dot)) { //handle ".()" or "range" or "dot"
      if (scan_peep_is_token(Token_id_op, 1)) {
        type = Lnast_ntype::create_func_call(); // must be a function call op
        walk_next_token();
        I(scan_peep_is_token(Token_id_cp, 1));
        walk_next_token();
      } else if (scan_peep_is_token(Token_id_dot, 1)) {
        type = Lnast_ntype::create_range();
        walk_next_token();
      } else {
        type = Lnast_ntype::create_dot();
      }
  } else if (scan_is_token(Token_id_obr)) {
    if (scan_peep_is_token(Token_id_obr, 1)) {
      type = Lnast_ntype::create_bit_select(); // must be a bit selection op
      walk_next_token();
      I(scan_peep_is_token(Token_id_cbr, 1));
      walk_next_token();
      I(scan_peep_is_token(Token_id_cbr, 1));
      walk_next_token();
    } else if (scan_peep_is_token(Token_id_cbr, 1)) {
      type = Lnast_ntype::create_select();
      walk_next_token();
    }
  } else if (scan_is_token(Token_id_plus)) {
    if (scan_peep_is_token(Token_id_plus, 1)){
      type = Lnast_ntype::create_tuple_concat();
      walk_next_token();
    } else {
      type = Lnast_ntype::create_plus();
    }
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "as") {
    type = Lnast_ntype::create_as();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "for") {
    type = Lnast_ntype::create_for();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "while") {
    type = Lnast_ntype::create_while();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "if") {
    type = Lnast_ntype::create_if();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "uif") {
    type = Lnast_ntype::create_uif();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "elif"){
    type = Lnast_ntype::create_elif();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "I") {
    type = Lnast_ntype::create_assert();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "and") {
    type = Lnast_ntype::create_logical_and();
  } else if (scan_is_token(Token_id_alnum) && scan_text() == "or") {
    type = Lnast_ntype::create_logical_or();
  } else if (scan_is_token(Token_id_eq)) {
    type = Lnast_ntype::create_assign();
  } else if (scan_is_token(Token_id_and)) {
    type = Lnast_ntype::create_and();
  } else if (scan_is_token(Token_id_or)) {
    type = Lnast_ntype::create_or();
  } else if (scan_is_token(Token_id_xor)) {
    type = Lnast_ntype::create_xor();
  } else if (scan_is_token(Token_id_minus)) {
    type = Lnast_ntype::create_minus();
  } else if (scan_is_token(Token_id_mult)) {
    type = Lnast_ntype::create_mult();
  } else if (scan_is_token(Token_id_div)) {
    type = Lnast_ntype::create_div();
  } else if (scan_is_token(Token_id_same)) {
    type = Lnast_ntype::create_same();
  } else if (scan_is_token(Token_id_le)) {
    type = Lnast_ntype::create_le();
  } else if (scan_is_token(Token_id_ge)) {
    type = Lnast_ntype::create_ge();
  } else {
    I(type.is_invalid());
  }
  return type;
}

bool Lnast_parser::token_is_valid_ref() {
  return (scan_is_token(Token_id_alnum) || scan_is_token(Token_id_output) || scan_is_token(Token_id_input) || scan_is_token(Token_id_register));
}


