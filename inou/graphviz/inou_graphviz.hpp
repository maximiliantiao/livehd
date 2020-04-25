//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#pragma once

#include <memory>

#include "lgraph.hpp"
#include "pass.hpp"

class Inou_graphviz : public Pass {
private:
  static void populate_lg_handle_xedge(const Node &node, const XEdge &out, std::string &data);

protected:
  // bits/verbose are optional arguments to Inou_graphviz pass
  bool bits;
  bool verbose;

  void do_hierarchy(LGraph *lg);
  void do_from_lgraph(LGraph *lg);
  void populate_lg_data(LGraph *lg);

  void do_from_lnast(std::shared_ptr<Lnast> lnast);

  void do_fromfirrtl(Eprp_var &var);

  // eprp callback methods
  static void from(Eprp_var &var);
  static void fromlnast(Eprp_var &var);
  static void fromfirrtl(Eprp_var &var);
  static void hierarchy(Eprp_var &var);

public:
  Inou_graphviz(const Eprp_var &var);

  static void setup();
};
