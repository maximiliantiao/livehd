//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#pragma once

#include <string>
#include <tuple>
#include <stack>

#include "pass.hpp"
#include "lnast.hpp"
#include "lconst.hpp"
#include "mmap_tree.hpp"
#include "firrtl.pb.h"

class Inou_firrtl : public Pass {
protected:
  //----------- FOR toLNAST ----------
  std::string_view create_temp_var(Lnast& lnast);
  std::string_view get_new_seq_name(Lnast& lnast);
  std::string      get_full_name(std::string term, const bool is_rhs);

  // Helper Functions (for handling specific cases)
  void create_bitwidth_dot_node(Lnast &lnast, uint32_t bw, Lnast_nid& parent_node, std::string port_id);
  int  get_bit_count      (const firrtl::FirrtlPB_Type type);
  void init_wire_dots     (Lnast &lnast, const firrtl::FirrtlPB_Statement_Wire& expr, Lnast_nid& parent_node);
  void init_register_dots (Lnast &lnast, const firrtl::FirrtlPB_Statement_Register& expr, Lnast_nid& parent_node);
  void create_module_inst (Lnast &lnast, const firrtl::FirrtlPB_Statement_Instance& inst, Lnast_nid& parent_node);
  void HandleMuxAssign    (Lnast &lnast, const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, const std::string lhs_of_asg);
  void HandleValidIfAssign(Lnast &lnast, const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, const std::string lhs_of_asg);
  void HandleNEQOp        (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleUnaryOp      (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleAndReducOp   (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleOrReducOp    (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleXorReducOp   (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleNegateOp     (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleExtractBitsOp(Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleHeadOp       (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleTailOp       (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleConcatOp     (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandlePadOp        (Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleTwoExprPrimOp(Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void HandleStaticShiftOp(Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, const std::string lhs);
  void AttachExprStrToNode(Lnast &lnast, const std::string_view access_str, Lnast_nid& parent_node);

  std::string HandleSubfieldAcc(Lnast &lnast, const firrtl::FirrtlPB_Expression_SubField sub_field, Lnast_nid& parent_node, const bool is_rhs);
  std::string CreateNameStack  (const firrtl::FirrtlPB_Expression_SubField sub_field, std::stack<std::string>& names);

  // Deconstructing Protobuf Hierarchy
  void create_io_list(const firrtl::FirrtlPB_Type& type, uint8_t dir, std::string port_id,
                        std::vector<std::tuple<std::string, uint8_t, uint32_t>>& vec);
  void ListPortInfo(Lnast &lnast, const firrtl::FirrtlPB_Port& port, Lnast_nid parent_node);

  void PrintPrimOp(Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, const std::string symbol, Lnast_nid& parent_node);
  void ListPrimOpInfo(Lnast &lnast, const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void InitialExprAdd(Lnast &lnast, const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, std::string lhs_unalt);
  std::string ReturnExprString(Lnast &lnast, const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, const bool is_rhs);


  void ListStatementInfo(Lnast &lnast, const firrtl::FirrtlPB_Statement& stmt, Lnast_nid& parent_node);

  void ListUserModuleInfo(Eprp_var &var, const firrtl::FirrtlPB_Module& module);
  void ListModuleInfo(Eprp_var &var, const firrtl::FirrtlPB_Module& module);
  void IterateModules(Eprp_var &var, const firrtl::FirrtlPB_Circuit& circuit);
  void IterateCircuits(Eprp_var &var, const firrtl::FirrtlPB& firrtl_input);

  static void toLNAST(Eprp_var &var);


  //----------- FOR toFIRRTL ----------
  static void toFIRRTL            (Eprp_var &var);
  void        do_tofirrtl         (std::shared_ptr<Lnast> ln);
  void        process_ln_stmt     (Lnast &ln, const Lnast_nid &lnidx_smts, firrtl::FirrtlPB_Statement* fstmt);
  void        process_ln_assign_op(Lnast &ln, const Lnast_nid &lnidx_assign, firrtl::FirrtlPB_Statement* fstmt);
  void        process_ln_nary_op  (Lnast &ln, const Lnast_nid &lnidx_assign, firrtl::FirrtlPB_Statement* fstmt);
  void        process_ln_not_op   (Lnast &ln, const Lnast_nid &lnidx_op, firrtl::FirrtlPB_Statement* fstmt);
  void        process_ln_if_op    (Lnast &ln, const Lnast_nid &lnidx_if);
  void        process_ln_phi_op   (Lnast &ln, const Lnast_nid &lnidx_phi);

  uint8_t     process_op_children (Lnast &ln, const Lnast_nid &lnidx_if, const std::string firrtl_op);

  // Helper Functions
  bool        is_inp                (const std::string_view str);
  bool        is_outp               (const std::string_view str);
  bool        is_reg                (const std::string_view str);
  void        create_connect_stmt   (Lnast &ln, const Lnast_nid &lhs, firrtl::FirrtlPB_Expression* rhs_expr,
                                     firrtl::FirrtlPB_Statement* fstmt);
  void        create_node_stmt      (Lnast &ln, const Lnast_nid &lhs, firrtl::FirrtlPB_Expression* rhs_expr,
                                     firrtl::FirrtlPB_Statement* fstmt);
  void        create_integer_object (Lnast &ln, const Lnast_nid &lnidx_const, firrtl::FirrtlPB_Expression* rhs_expr);
  std::string get_firrtl_name_format(Lnast &ln, const Lnast_nid &lnidx);
  std::string strip_prefixes        (const std::string_view str);
  std::string create_const_token    (const std::string_view str);
  void        add_const_or_ref_to_primop (Lnast &ln, const Lnast_nid &lnidx, firrtl::FirrtlPB_Expression_PrimOp* prim_op);
  firrtl::FirrtlPB_Expression_PrimOp_Op  get_firrtl_oper_code(const Lnast_ntype &op_type);

private:
  //----------- FOR toLNAST ----------
  std::vector<std::string> input_names;
  std::vector<std::string> output_names;
  std::vector<std::string> register_names;

  uint32_t temp_var_count;
  uint32_t seq_counter;

public:
  Inou_firrtl(const Eprp_var &var);

  static void setup();
};
