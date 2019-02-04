#include "trie.h"

#include <deque>

using std::deque;
using std::string;
using std::vector;

namespace trie {

FastTrie::FastTrie(const vector<string> &keys) {
  for (const auto &key : keys) {
    auto *current_node = &root;

    for (const char character : key) {
      if (current_node->children[character] == nullptr)
        current_node->children[character] = std::make_unique<TrieNode>();

      current_node = current_node->children[character].get();
    }

    current_node->is_end_of_word = true;
  }
}

bool FastTrie::search(const string &key) const {
  auto *current_node = &root;

  for (const char character : key) {
    if (!current_node->children[character]) return false;

    current_node = current_node->children[character].get();
  }

  return (current_node != nullptr && current_node->is_end_of_word);
}

SerializedTrie::SerializedTrie(const vector<string> &keys) {
  struct IndexedTrieNode {
    FastTrie::TrieNode const *const pnode;
    int16_t const idx;
    IndexedTrieNode(FastTrie::TrieNode const *const node, int16_t index)
        : pnode(node), idx(index) {}
  };

  const FastTrie fast_trie(keys);

  deque<IndexedTrieNode> to_visit;
  to_visit.emplace_back(fast_trie.getRoot(), -1);
  while (!to_visit.empty()) {
    auto [node, idx] = to_visit.front();
    to_visit.pop_front();

    bool has_children = false;
    for (int i = 0; i < alphabet_size; ++i) {
      if (node->children[i] != nullptr) {
        if (idx >= 0 && nodes[idx].children_offset < 0) {
          nodes[idx].children_offset =
              static_cast<uint16_t>(nodes.size() - idx);
        }
        nodes.emplace_back(static_cast<char>(i),
                           node->children[i]->is_end_of_word);

        to_visit.emplace_back(node->children[i].get(),
                              static_cast<uint16_t>(nodes.size()) - 1);

        has_children = true;
      }
    }
    if (has_children) {
      nodes.emplace_back(terminating_character);
    }
  }
}

bool SerializedTrie::search(const string &key) const {
  int curr_node = 0;

  for (int i = 0; i < key.size(); ++i) {
    if (i != 0) curr_node += nodes[curr_node].children_offset;

    while (nodes[curr_node].character != terminating_character &&
           nodes[curr_node].character != key[i])
      curr_node++;
    if (nodes[curr_node].character == terminating_character) return false;
  }

  return nodes[curr_node].is_leaf;
}

}  // namespace trie
