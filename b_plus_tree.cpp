#include "include/b_plus_tree.h"
#include <iostream>
using std::cout;

bool BPlusTree::IsEmpty() const {
    if(root==nullptr || root->key_num==0) return true;
    return false;
}

bool BPlusTree::GetValue(const KeyType &key, RecordPointer &result) {
    if(IsEmpty()) return false;

    Node *temp=findLeafNode(root,key);
    for(int pos=0; pos<temp->key_num; pos++){
        if(temp->keys[pos]==key){
            result = ((LeafNode*)temp)->pointers[pos];
            return true;
        }
    }
    return false;
}

Node *BPlusTree::findLeafNode(Node *head, const KeyType &key){
    while(!head->is_leaf){
        int pos=0;
        for(pos=0; pos<head->key_num; pos++){
            if(head->keys[pos]>key) break;
        }
        head=((InternalNode*)head)->children[pos];
    }
    return head;
}

bool BPlusTree::Insert(const KeyType &key, const RecordPointer &value) {
    // Case when no element present
    if(IsEmpty()){
        auto *leaf = new LeafNode();
        leaf->keys[leaf->key_num]=key;
        leaf->pointers[leaf->key_num++]=value;
        root=leaf;
        return true;
    }

    // find leaf node w.r.t. to key
    auto *temp=(LeafNode*)findLeafNode(root,key);

    // return false if key exists
    for(int pos=0; pos<temp->key_num; pos++){
        if(temp->keys[pos]==key) return false;
    }

    int pos;
    for(pos=0; pos<temp->key_num; pos++){
        if(temp->keys[pos]>key) break;
    }

    // when leaf has empty space to add record
    if(temp->key_num<MAX_FANOUT-1){
        for(int i=temp->key_num; i>pos; i--){
            temp->pointers[i]=temp->pointers[i-1];
            temp->keys[i]=temp->keys[i-1];
        }
        temp->pointers[pos]=value;
        temp->keys[pos]=key;
        temp->key_num++;
        return true;
    }
    // when leaf is full
    else{
        auto *leaf = new LeafNode();
        int split=(MAX_FANOUT+1)/2;
        temp->key_num=split;
        leaf->key_num=MAX_FANOUT-split;

        leaf->prev_leaf=temp;
        leaf->next_leaf=temp->next_leaf;
        if(temp->next_leaf) temp->next_leaf->prev_leaf=leaf;
        temp->next_leaf=leaf;

        // when record to be inserted in current leaf node
        if(pos<split){
            int i,j;
            for(i=split-1, j=0; i<MAX_FANOUT-1; i++, j++){
                leaf->pointers[j]=temp->pointers[i];
                leaf->keys[j]=temp->keys[i];
                temp->pointers[i]=RecordPointer();
                temp->keys[i]=0;
            }
            for(i=split-1; i>pos; i--){
                temp->pointers[i]=temp->pointers[i-1];
                temp->keys[i]=temp->keys[i-1];
            }
            temp->keys[pos]=key;
            temp->pointers[pos]=value;
        }
        // when record to be inserted in new leaf node
        else{
            int i,j;
            for(i=split, j=0; i<MAX_FANOUT-1; i++, j++){
                leaf->pointers[j]=temp->pointers[i];
                leaf->keys[j]=temp->keys[i];
                temp->pointers[i]=RecordPointer();
                temp->keys[i]=0;
            }
            pos=pos-split;
            for(i=j; i>pos; i--){
                leaf->pointers[i]=leaf->pointers[i-1];
                leaf->keys[i]=leaf->keys[i-1];
            }
            leaf->pointers[pos]=value;
            leaf->keys[pos]=key;
        }
        modifyParentAfterInsertion(key, temp, leaf, leaf->keys[0]);
        return true;
    }
}

// recursively modify parent after insertion
void BPlusTree::modifyParentAfterInsertion(const KeyType &posKey, Node *n1, Node *n2, const KeyType &propagatedKey){
    // when parent of n1 is root, create new node
    if(n1->parent==nullptr){
        auto *internal = new InternalNode();
        internal->keys[internal->key_num++]=propagatedKey;
        internal->children[0]=n1; internal->children[1]=n2;
        n1->parent=internal; n2->parent=internal;
        root=internal;
        return;
    }

    auto *node=(InternalNode*)n1->parent;
    int pos;
    for(pos=0; pos<node->key_num; pos++){
        if(node->keys[pos]>posKey) break;
    }

    // when node can accommodate new key
    if(node->key_num<MAX_FANOUT-1){
        for(int i=node->key_num; i>pos; i--){
            node->children[i+1]=node->children[i];
            node->keys[i]=node->keys[i-1];
        }
        node->children[pos+1]=n2;
        node->keys[pos]=propagatedKey;
        n2->parent=node;
        node->key_num++;
        return;
    }
    // when node is full, create new node 'internal'
    else{
        auto *internal = new InternalNode();
        int split=(MAX_FANOUT+1)/2;
        node->key_num=split-1;
        internal->key_num=MAX_FANOUT-split;

        // 3 cases for where to append propagatedKey: node, internal or further propagate up
        if(pos==split-1){
            int i,j;
            for(i=split-1, j=0; i<MAX_FANOUT-1; i++, j++){
                internal->children[j+1]=node->children[i+1];
                internal->keys[j]=node->keys[i];
                internal->children[j+1]->parent=internal;
                node->children[i+1]= nullptr;
                node->keys[i]=0;
            }
            internal->children[0]=n2;
            internal->children[0]->parent=internal;

            modifyParentAfterInsertion(posKey, node, internal, propagatedKey);
        }
        else if(pos<split-1){
            int i,j;
            for(i=split-1, j=0; i<MAX_FANOUT-1; i++, j++){
                internal->children[j]=node->children[i];
                internal->keys[j]=node->keys[i];
                internal->children[j]->parent=internal;
                node->children[i]=nullptr;
                node->keys[i]=0;
            }
            internal->children[j]=node->children[i];
            node->children[i]=nullptr;
            internal->children[j]->parent=internal;

            KeyType newKey=node->keys[split-2];

            for(i=split-2; i>pos; i--){
                node->children[i+1]=node->children[i];
                node->keys[i]=node->keys[i-1];
            }
            node->children[pos+1]=n2;
            node->keys[pos]=propagatedKey;
            node->children[pos+1]->parent=node;

            modifyParentAfterInsertion(posKey, node, internal, newKey);
        }
        else{
            int i,j;
            for(i=split, j=0; i<MAX_FANOUT-1; i++, j++){
                internal->children[j]=node->children[i];
                internal->keys[j]=node->keys[i];
                internal->children[j]->parent=internal;
                node->children[i]= nullptr;
                node->keys[i]=0;
            }
            internal->children[j]=node->children[i];
            node->children[i]=nullptr;
            internal->children[j]->parent=internal;

            KeyType newKey=node->keys[split-1];
            node->keys[split-1]=0;

            pos=pos-split;
            for(i=j; i>pos; i--){
                internal->children[i+1]=internal->children[i];
                internal->keys[i]=internal->keys[i-1];
            }
            internal->children[pos+1]=n2;
            internal->keys[pos]=propagatedKey;
            internal->children[pos+1]->parent=internal;

            modifyParentAfterInsertion(posKey, node, internal, newKey);
        }
    }
}

void BPlusTree::Remove(const KeyType &key) {
    // if tree empty or element doesn't exist in tree;
    if(IsEmpty()) return;
    RecordPointer p;
    if(!GetValue(key, p)) return;

    // when root is leaf
    if(root->is_leaf){
        auto *leaf=(LeafNode*)root;

        // if root is leaf & leaf has 1 element
        if(leaf->key_num==1){
            delete(leaf);
            leaf=nullptr;
            return;
        }
        // when root is leaf & leaf has more than 1 element
        int pos,i;
        for(pos=0; pos<leaf->key_num; pos++){
            if(leaf->keys[pos]==key) break;
        }
        for(i=pos; i<leaf->key_num-1; i++){
            leaf->pointers[i]=leaf->pointers[i+1];
            leaf->keys[i]=leaf->keys[i+1];
        }
        leaf->pointers[i]=RecordPointer();
        leaf->keys[i]=0;
        leaf->key_num--;
        return;
    }
    Node *leaf=findLeafNode(root,key);
    removeEntryFromLeaf(leaf,key);
}

// remove an entry when node is a leaf
void BPlusTree::removeEntryFromLeaf(Node *node, const KeyType &key) {
    auto *leaf=(LeafNode*)node;

    // Delete key and pointer from leaf
    int pos,i,j;
    for(pos=0; pos<leaf->key_num; pos++){
        if(leaf->keys[pos]==key) break;
    }

    for(i=pos; i<leaf->key_num-1; i++){
        leaf->pointers[i]=leaf->pointers[i+1];
        leaf->keys[i]=leaf->keys[i+1];
    }
    leaf->pointers[i]=RecordPointer();
    leaf->keys[i]=0;
    leaf->key_num--;

    int minKeys=MAX_FANOUT/2 ;    // only for leaf nodes

    // when leaf has fewer key min required: merge or redistribute
    if (leaf->key_num<minKeys) {
        KeyType parentKey=0;
        bool siblingIsNext=false;
        auto *sibling=(LeafNode*)findSibling(leaf,key,&parentKey,&siblingIsNext);

        // merge if total size can be accommodated in one node
        if(leaf->key_num+sibling->key_num<MAX_FANOUT){
            if(siblingIsNext){
                j=leaf->key_num;
                for(i=sibling->key_num-1; i>=0; i--){
                    sibling->pointers[i+j]=sibling->pointers[i];
                    sibling->keys[i+j]=sibling->keys[i];
                }
                for(i=0; i<leaf->key_num; i++){
                    sibling->pointers[i]=leaf->pointers[i];
                    sibling->keys[i]=leaf->keys[i];
                }
                if(leaf->prev_leaf) leaf->prev_leaf->next_leaf=sibling;
                sibling->prev_leaf=leaf->prev_leaf;

                sibling->key_num+=leaf->key_num;
                removeEntryFromInternal(sibling->parent, parentKey);
                delete(leaf);
            }
            else{
                j=sibling->key_num;
                for(i=leaf->key_num-1; i>=0; i--){
                    leaf->pointers[i+j]=leaf->pointers[i];
                    leaf->keys[i+j]=leaf->keys[i];
                }
                for(i=0; i<sibling->key_num; i++){
                    leaf->pointers[i]=sibling->pointers[i];
                    leaf->keys[i]=sibling->keys[i];
                }
                if(sibling->prev_leaf) sibling->prev_leaf->next_leaf=leaf;
                leaf->prev_leaf=sibling->prev_leaf;

                leaf->key_num+=sibling->key_num;
                removeEntryFromInternal(leaf->parent, parentKey);
                delete(sibling);
            }
        }
        // else redistribute
        else{
            if(siblingIsNext){
                pos=leaf->key_num;

                leaf->pointers[pos]=sibling->pointers[0];
                leaf->keys[pos]=sibling->keys[0];
                leaf->key_num++;

                for(i=0; i<sibling->key_num-1; i++){
                    sibling->pointers[i]=sibling->pointers[i+1];
                    sibling->keys[i]=sibling->keys[i+1];
                }
                sibling->pointers[i]=RecordPointer();
                sibling->keys[i]=0;
                sibling->key_num--;

                replaceInParent(sibling->parent,parentKey,sibling->keys[0]);
            }
            else{
                pos=sibling->key_num-1;

                for(i=leaf->key_num-1; i>=0; i--){
                    leaf->pointers[i+1]=leaf->pointers[i];
                    leaf->keys[i+1]=leaf->keys[i];
                }
                leaf->pointers[0]=sibling->pointers[pos];
                leaf->keys[0]=sibling->keys[pos];
                leaf->key_num++;

                sibling->pointers[pos]=RecordPointer();
                sibling->keys[pos]=0;
                sibling->key_num--;

                replaceInParent(sibling->parent,parentKey,leaf->keys[0]);
            }
        }
    }
}

// remove an entry when node is not a leaf
void BPlusTree::removeEntryFromInternal(Node *node, const KeyType &key) {
    auto *internal=(InternalNode*)node;

    // if internal is root and has 1 pointer, make that pointer as root
    if(root==internal && internal->key_num==1){
        root=internal->children[1];
        delete(internal);
        return;
    }

    // Delete key and pointer from internal
    int pos,i,j;
    for(pos=0; pos<internal->key_num; pos++){
        if(internal->keys[pos]==key) break;
    }
    for(i=pos; i<internal->key_num-1; i++){
        internal->children[i]=internal->children[i+1];
        internal->keys[i]=internal->keys[i+1];
    }
    internal->children[i]=internal->children[i+1];
    internal->keys[i]=0;
    internal->children[i+1]=nullptr;
    internal->key_num--;

    int minKeys=(MAX_FANOUT-1)/2 ;    // only for internal nodes

    // when internal has fewer values than min required: merge or redistribute
    if (root!=internal && internal->key_num<minKeys){
        KeyType parentKey=0;
        bool siblingIsNext=false;
        auto *sibling=(InternalNode*)findSibling(internal,key,&parentKey,&siblingIsNext);

        // merge if total size can be accommodated in one node
        if(internal->key_num+sibling->key_num+1<MAX_FANOUT){
            if(siblingIsNext){
                j=internal->key_num+1;
                for(i=sibling->key_num-1; i>=0; i--){
                    sibling->children[i+j+1]=sibling->children[i+1];
                    sibling->keys[i+j]=sibling->keys[i];
                }
                sibling->children[i+j+1]=sibling->children[i+1];
                sibling->keys[i+j]=parentKey;

                for(i=0; i<internal->key_num; i++){
                    sibling->children[i]=internal->children[i];
                    sibling->keys[i]=internal->keys[i];
                    sibling->children[i]->parent=sibling;
                }
                sibling->children[i]=internal->children[i];
                sibling->children[i]->parent=sibling;

                sibling->key_num+=internal->key_num+1;
                removeEntryFromInternal(sibling->parent, parentKey);
                delete(internal);
            }
            else{
                j=sibling->key_num+1;
                for(i=internal->key_num-1; i>=0; i--){
                    internal->children[i+j+1]=internal->children[i+1];
                    internal->keys[i+j]=internal->keys[i];
                }
                internal->children[i+j+1]=internal->children[i+1];
                internal->keys[i+j]=parentKey;

                for(i=0; i<sibling->key_num; i++){
                    internal->children[i]=sibling->children[i];
                    internal->keys[i]=sibling->keys[i];
                    internal->children[i]->parent=internal;
                }
                internal->children[i]=sibling->children[i];
                internal->children[i]->parent=internal;

                internal->key_num+=sibling->key_num+1;
                removeEntryFromInternal(internal->parent, parentKey);
                delete(sibling);
            }
        }
        // else redistribute
        else{
            if(siblingIsNext){
                pos=internal->key_num;

                internal->children[pos+1]=sibling->children[0];
                internal->keys[pos]=parentKey;
                internal->children[pos+1]->parent=internal;
                internal->key_num++;

                KeyType newKey=sibling->keys[0];
                for(i=0; i<sibling->key_num-1; i++){
                    sibling->children[i]=sibling->children[i+1];
                    sibling->keys[i]=sibling->keys[i+1];
                }
                sibling->children[i]=sibling->children[i+1];

                sibling->children[i+1]=nullptr;
                sibling->keys[i]=0;
                sibling->key_num--;

                replaceInParent(sibling->parent, parentKey, newKey);
            }
            else{
                pos=sibling->key_num-1;

                for(i=internal->key_num-1; i>=0; i--){
                    internal->children[i+1+1]=internal->children[i+1];
                    internal->keys[i+1]=internal->keys[i];
                }
                internal->children[i+1+1]=internal->children[i+1];

                internal->children[0]=sibling->children[pos+1];
                internal->keys[0]=parentKey;
                internal->children[0]->parent=internal;
                internal->key_num++;

                KeyType newKey=sibling->keys[pos];
                sibling->children[pos+1]=nullptr;
                sibling->keys[pos]=0;
                sibling->key_num--;

                replaceInParent(sibling->parent, parentKey, newKey);
            }
        }
    }
}

// find immediate sibling: left or right
Node *BPlusTree::findSibling(Node *node, const KeyType &key, KeyType *parentKey, bool *siblingIsNext){
    auto *parent=(InternalNode*)node->parent;
    int pos=0;
    for(pos=0; pos<parent->key_num; pos++){
        if(parent->keys[pos]>key) break;
    }
    if(pos==0){
        *siblingIsNext=true;
        *parentKey=parent->keys[pos];
        return parent->children[pos+1];
    }
    else{
        *siblingIsNext=false;
        *parentKey=parent->keys[pos-1];
        return parent->children[pos-1];
    }
}

// replace a key in parent node with a new key
void BPlusTree::replaceInParent(Node *node, KeyType &parentKey, KeyType &replace){
    for(int i=0; i<node->key_num; i++){
        if(node->keys[i]==parentKey){
            node->keys[i]=replace;
            return;
        }
    }
}

void BPlusTree::RangeScan(const KeyType &key_start, const KeyType &key_end,
                          std::vector<RecordPointer> &result) {
    if(IsEmpty()) return;

    auto *temp=(LeafNode*)findLeafNode(root,key_start);
    int pos;
    for(pos=0; pos<temp->key_num; pos++){
        if(temp->keys[pos]>=key_start) break;
    }

    bool flag=true;
    while(flag){
        while(pos<temp->key_num){
            if(temp->keys[pos]>=key_end){ flag=false; break; }
            result.push_back(temp->pointers[pos++]);
        }
        pos=0;
        if(temp->next_leaf) temp=temp->next_leaf;
        else break;
    }
}
