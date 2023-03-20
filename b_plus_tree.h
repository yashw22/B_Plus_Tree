#pragma once
#include <queue>
#include <string>
#include <vector>
#include "para.h"
using namespace std;

struct RecordPointer {
  int page_id;
  int record_id;
  RecordPointer() : page_id(0), record_id(0){};
  RecordPointer(int page, int record) : page_id(page), record_id(record){};
};

class Node {
public:
  Node(bool leaf) : key_num(0), is_leaf(leaf) { fill(keys, keys+MAX_FANOUT-1, 0);};
  bool is_leaf;
  int key_num;
  KeyType keys[MAX_FANOUT - 1];
  Node* parent = nullptr;
};

class InternalNode : public Node {
public:
  InternalNode() : Node(false) { fill(children, children+MAX_FANOUT, nullptr); };
  Node * children[MAX_FANOUT];
};

class LeafNode : public Node {
public:
  LeafNode() : Node(true) { fill(pointers, pointers+MAX_FANOUT-1, RecordPointer(0,0) ); };
  RecordPointer pointers[MAX_FANOUT - 1];
  LeafNode *next_leaf = nullptr;
  LeafNode *prev_leaf = nullptr;
};

class BPlusTree {
public:
  BPlusTree(): root(nullptr){};

  bool IsEmpty() const;
  bool Insert(const KeyType &key, const RecordPointer &value);
  void Remove(const KeyType &key);
  bool GetValue(const KeyType &key, RecordPointer &result);
  void RangeScan(const KeyType &key_start, const KeyType &key_end,
                 std::vector<RecordPointer> &result);

private:
  Node *findLeafNode(Node *head, const KeyType &key);
  void modifyParentAfterInsertion(const KeyType &posKey, Node *n1, Node *n2, const KeyType &propagatedKey);
  void removeEntryFromLeaf(Node *node, const KeyType &key);
  void removeEntryFromInternal(Node *node, const KeyType &key);
  Node *findSibling(Node *node, const KeyType &key, KeyType *parentKey, bool *siblingIsNext);
  void replaceInParent(Node *node, KeyType &parentKey, KeyType &replace);

  Node *root;
};
