#include <memory>
#include <string>
#include <vector>

namespace trie {

static constexpr int alphabet_size = std::numeric_limits<char>::max() + 1;

class FastTrie {
  struct TrieNode {
    std::vector<std::unique_ptr<TrieNode>> children =
        std::vector<std::unique_ptr<TrieNode>>(alphabet_size);
    bool is_end_of_word = false;
  };

  TrieNode root;
  TrieNode const *const getRoot() const { return &root; }
  friend class SerializedTrie;

 public:
  FastTrie(const std::vector<std::string> &keys);
  bool search(const std::string &key) const;
};

class SerializedTrie {
  static constexpr char terminating_character = '\n';

  struct SerialTrieNode {
    int16_t children_offset = -1;
    char character = terminating_character;
    bool is_leaf = false;
    explicit SerialTrieNode(char c, bool leaf = false) noexcept
        : character(c), is_leaf(leaf) {}
  };

  std::vector<SerialTrieNode> nodes;

 public:
  explicit SerializedTrie(const std::vector<std::string> &keys);
  bool search(const std::string &key) const;
};

}  // namespace trie
