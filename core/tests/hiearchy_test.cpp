//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <vector>
#include <string>

#include "rng.hpp"
#include "tree.hpp"

#include "attribute.hpp"
#include "lgedgeiter.hpp"
#include "lgraph.hpp"

#include "tree_lgdb_setup.hpp"

using testing::HasSubstr;

class Setup_hierarchy : public Tree_lgdb_setup {
protected:

  void SetUp() override {
  }

  void TearDown() override {
    //Graph_library::sync_all();
  }
};

TEST_F(Setup_hierarchy, shallow_tree) {

  int rseed=123;
  populate_tree(rseed, 4, 100, 0.5, true);
  map_tree_to_lgraph(rseed);

  for (const auto &index : tree.breadth_first()) {
    const auto &data = tree.get_data(index);
    Node node(top_lg, data.cnode);

    if (index.level == 0) {
      EXPECT_EQ(node.get_top_lgraph(), node.get_class_lgraph());
      EXPECT_EQ(node.get_top_lgraph(), top_lg);
    } else {
      auto parent_index = tree.get_parent(index);
      const auto &parent_data = tree.get_data(parent_index);
      Node parent_node(top_lg, parent_data.cnode);

      I(parent_node.is_type_sub());

      EXPECT_EQ(parent_node.hierarchy_go_down(), node.get_hidx());
      EXPECT_EQ(node.hierarchy_go_up(), parent_node.get_hidx());

      EXPECT_EQ(parent_node.get_type_sub_lgraph(), node.get_class_lgraph());
      EXPECT_EQ(node.get_parent_lgraph(), parent_node.get_class_lgraph());
    }
  }
}
