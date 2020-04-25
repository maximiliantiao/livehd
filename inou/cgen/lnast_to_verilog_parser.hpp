#pragma once

#include "cgen_variable_manager.hpp"
#include "lnast_to_xxx.hpp"
#include "verilog_parser_module.hpp"

class Lnast_to_verilog_parser : public Lnast_to_xxx {
private:
  // infrastructure to process the nodes
  mmap_lib::Tree_level                 curr_statement_level = -1;
  mmap_lib::Tree_level                 prev_statement_level = -1;
  std::vector<mmap_lib::Tree_level>    level_stack;
  std::vector<std::vector<Lnast_node>> buffer_stack;
  std::vector<Lnast_node>              node_buffer;

  // infrastructure for multiple modules
  std::string            root_filename;
  Verilog_parser_module *curr_module;
  // key, pair(value, variables)
  std::map<std::string_view, std::string> ref_map;
  std::vector<Verilog_parser_module *>    module_stack;

  // references
  std::string_view                               get_node_name(Lnast_node node);
  std::string                                    get_filename(std::string filepath);
  std::map<std::string, Verilog_parser_module *> func_map;

  // infrastructure
  void process_node(const mmap_lib::Tree_index &it);
  void process_top(mmap_lib::Tree_level level);
  void push_statement(mmap_lib::Tree_level level, Lnast_ntype type);  // prepare for next statement
  void pop_statement();
  void flush_stmts();
  void add_to_buffer(Lnast_node node);
  void process_buffer();

  bool             is_number(std::string_view test_string);
  std::string_view process_number(std::string_view num);
  bool             is_ref(std::string_view test_string);
  bool             is_attr(std::string_view test_string);

  void process_assign();
  void process_as();
  void process_label();

  void process_if();
  void process_func_call();
  void process_func_def();

  void process_operator();

public:
  Lnast_to_verilog_parser(std::shared_ptr<Lnast> _lnast, std::string_view _path) : Lnast_to_xxx(_lnast, _path){};

  void generate() final;
};
