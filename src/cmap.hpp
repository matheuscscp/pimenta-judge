#ifndef CMAP_H
#define CMAP_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

template <typename K, typename M>
class cmap {
  // implementation
  private:
    struct node_;
    typedef node_* node;
  // API
  public:
    typedef std::pair<const K,M> V;
    class iterator {
      public:
        V* operator->() {
          return &u->v;
        }
        V& operator*() {
          return u->v;
        }
        iterator& operator++() {
          inc();
          return *this;
        }
        iterator operator++(int) {
          iterator ans(*this);
          inc();
          return ans;
        }
        bool operator==(const iterator& o) const {
          return u == o.u;
        }
        bool operator!=(const iterator& o) const {
          return u != o.u;
        }
      private:
        friend class cmap;
        node u;
        iterator(node u) : u(u) {}
        void inc() {
          if (!u) return;
          if (u->r) {
            u = u->r;
            while (u && u->l) u = u->l;
            return;
          }
          node last;
          do {
            last = u;
            u = u->p;
          } while (u && last == u->r);
        }
    };
    cmap() : root(nullptr) {}
    ~cmap() { clear(); }
    cmap(const cmap& o) : root(nullptr) { operator=(o); }
    cmap& operator=(const cmap& o) {
      clear();
      insert(copy(o.root));
      return *this;
    }
    cmap(cmap&& o) : root(nullptr) { operator=(std::move(o)); }
    cmap& operator=(cmap&& o) {
      std::swap(root,o.root);
      return *this;
    }
    M& operator[](const K& x) {
      node tn = search(x);
      if (!tn) {
        tn = new node_(K(x));
        insert(tn);
      }
      return tn->v.second;
    }
    M& operator[](K&& x) {
      node tn = search(x);
      if (!tn) {
        tn = new node_(std::move(x));
        insert(tn);
      }
      return tn->v.second;
    }
    iterator find(const K& x) {
      return iterator(search(x));
    }
    iterator at(size_t i) {
      if (!root || root->cnt <= i) return iterator(nullptr);
      node ans = root;
      size_t l = 0, r = root->cnt-1;
      for (size_t cur = l+cnt(ans->l); i != cur; cur = l+cnt(ans->l)) {
        if (i < cur) {
          r = cur-1;
          ans = ans->l;
        }
        else {
          l = cur+1;
          ans = ans->r;
        }
      }
      return iterator(ans);
    }
    iterator erase(iterator position) {
      iterator ans = position;
      ans++;
      remove(position.u->v.first);
      return ans;
    }
    size_t erase(const K& x) {
      size_t cur = size();
      remove(x);
      return size()-cur;
    }
    void clear() {
      clear(root);
    }
    size_t size() const {
      return cnt(root);
    }
    iterator begin() {
      node ans;
      for (ans = root; ans && ans->l; ans = ans->l);
      return iterator(ans);
    }
    iterator end() {
      return iterator(nullptr);
    }
    K max_key() const {
      K ans;
      node tmp;
      for (tmp = root; tmp && tmp->r; tmp = tmp->r);
      if (tmp) ans = tmp->v.first;
      return ans;
    }
  // implementation
  private:
    struct node_ {
      V v;
      size_t y,cnt;
      node_* p;
      node_* l;
      node_* r;
      node_(K&& x) : v(std::move(x),std::move(M())), l(nullptr), r(nullptr) {
        for (int i = 0, n = sizeof y; i < n; i++) {
          *(uint8_t*)&y = rand()&0x0ff;
        }
      }
    };
    node root;
    size_t cnt(node t) const {
      return t ? t->cnt : 0;
    }
    void update(node t) {
      if (!t) return;
      t->cnt = cnt(t->l)+1+cnt(t->r);
      if (t->l) t->l->p = t;
      if (t->r) t->r->p = t;
    }
    node search(const K& x) const {
      node ans;
      for (
        ans = root;
        ans && (ans->v.first < x || x < ans->v.first);
        ans = (x < ans->v.first ? ans->l : ans->r)
      );
      return ans;
    }
    void insert(node tn) {
      insert(tn,root);
      if (root) root->p = nullptr;
    }
    void remove(const K& x) {
      remove(x,root);
      if (root) root->p = nullptr;
    }
    void clear(node& t) {
      if (!t) return;
      clear(t->l);
      clear(t->r);
      delete t;
      t = nullptr;
    }
    node copy(node t) {
      if (!t) return nullptr;
      node nt = new node_(K(t->v.first));
      nt->v.second = t->v.second;
      nt->y = t->y;
      nt->l = copy(t->l);
      nt->r = copy(t->r);
      update(nt);
      return nt;
    }
    void split(node t, const K& x, node& l, node& r) {
      if (!t)
        l = r = nullptr;
      else if (x < t->v.first)
        split(t->l,x,l,t->l), r = t;
      else  
        split(t->r,x,t->r,r), l = t;
      update(t);
    }
    void merge(node l, node r, node& t) {
      if (!l || !r)
        t = l ? l : r;
      else if (l->y > r->y)
        merge(l->r,r,l->r), t = l;
      else
        merge(l,r->l,r->l), t = r;
      update(t);
    }
    void insert(node tn, node& t) {
      if (!t)
        t = tn;
      else if (tn->y > t->y)
        split(t,tn->v.first,tn->l,tn->r), t = tn;
      else
        insert(tn,tn->v.first < t->v.first ? t->l : t->r);
      update(t);
    }
    void remove(const K& x, node& t) {
      if (!t) return;
      if (x < t->v.first || t->v.first < x) {
        remove(x,x < t->v.first ? t->l : t->r);
        update(t);
        return;
      }
      node l = t->l, r = t->r;
      delete t;
      merge(l,r,t);
    }
};

#endif
