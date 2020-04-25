//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#pragma once

#include <string>

#include "pass.hpp"
#include "lnast.hpp"
#include "mmap_tree.hpp"
#include "firrtl.pb.h"

class Inou_firrtl : public Pass {
protected:

  // Helper Functions (for handling specific cases)
  void CreateBitwidthAttribute(uint32_t bw, Lnast_nid& parent_node, std::string port_id);
  void HandleMuxAssign(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, std::string lhs_of_asg);
  void HandleValidIfAssign(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, std::string lhs_of_asg);
  void CreateConditionNode(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node);
  void CreateConditionNode(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, const std::string tail);
  void HandleNEQOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void HandleUnaryOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs, std::string sop);
  void HandleNegateOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void HandleExtractBitsOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void HandleHeadOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void HandleTailOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void HandlePadOp(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);

  // Deconstructing Protobuf Hierarchy
  void ListTypeInfo(const firrtl::FirrtlPB_Type& type, Lnast_nid& parent_node, std::string port_id);
  void ListPortInfo(const firrtl::FirrtlPB_Port& port, Lnast_nid parent_node);

  void PrintPrimOp(const firrtl::FirrtlPB_Expression_PrimOp& op, const std::string symbol, Lnast_nid& parent_node);
  void ListPrimOpInfo(const firrtl::FirrtlPB_Expression_PrimOp& op, Lnast_nid& parent_node, std::string lhs);
  void AttachExprToOperator(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node);
  void AttachExprToOperator(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, std::string tail);
  void InitialExprAdd(const firrtl::FirrtlPB_Expression& expr, Lnast_nid& parent_node, std::string lhs, std::string tail);
  std::string ReturnExprString(const firrtl::FirrtlPB_Expression& expr);


  void ListStatementInfo(const firrtl::FirrtlPB_Statement& stmt, Lnast_nid& parent_node);

  void ListUserModuleInfo(const firrtl::FirrtlPB_Module& module);
  void ListModuleInfo(const firrtl::FirrtlPB_Module& module);
  void IterateModules(const firrtl::FirrtlPB_Circuit& circuit);
  void IterateCircuits(const firrtl::FirrtlPB& firrtl_input);

  static void toLNAST(Eprp_var &var);

private:
  std::shared_ptr<Lnast> lnast;
  //std::vector<Lnast> lnast_vec;

  uint32_t id_counter;

public:
  Inou_firrtl(const Eprp_var &var);

  static void setup();

  std::shared_ptr<Lnast> ref_lnast() { return lnast; };//FIXME: Temporary workaround for graphviz to work (only works for 1 module)
};
