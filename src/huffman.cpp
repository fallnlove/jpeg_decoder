#include <huffman.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <vector>

#include <glog/logging.h>

class HuffmanTree::Impl {
public:
    Impl() = delete;

    Impl(const std::vector<uint8_t> &code_lengths, const std::vector<uint8_t> &values) {
        if (code_lengths.size() > 16 ||
            std::accumulate(code_lengths.begin(), code_lengths.end(), 0u) != values.size()) {
            DLOG(ERROR) << "Invalid Node\n";
            throw std::invalid_argument("");
        }

        tree_.emplace_back(0, 0, 0);

        size_t values_idx = 0;

        std::vector<uint8_t> code_lengths_copy = code_lengths;

        MakeTree(0, 0, code_lengths_copy, values, values_idx);

        for (auto i : code_lengths_copy) {
            if (i != 0) {
                DLOG(ERROR) << "Invalid Node\n";
                throw std::invalid_argument("");
            }
        }
    }

    bool Move(bool bit, int &value) {
        index_ = bit ? tree_[index_].right_ : tree_[index_].left_;
        if (index_ == 0) {
            DLOG(ERROR) << "Invalid Node\n";
            throw std::invalid_argument("");
        }
        if (tree_[index_].left_ == 0 && tree_[index_].right_ == 0) {
            value = static_cast<int>(tree_[index_].value_);
            index_ = 0;
            return true;
        }
        return false;
    }

public:
    struct Node {
        size_t left_;
        size_t right_;
        uint8_t value_;

        Node(size_t left, size_t right, uint8_t value) : left_(left), right_(right), value_(value) {
        }
    };

    std::vector<Node> tree_;
    size_t index_ = 0;

    void MakeTree(size_t idx, size_t depth, std::vector<uint8_t> &code_lengths,
                  const std::vector<uint8_t> &values, size_t &values_idx) {
        if (depth > 0 && code_lengths[depth - 1] > 0) {
            --code_lengths[depth - 1];
            tree_[idx].value_ = values[values_idx];
            ++values_idx;
            return;
        }

        if (values_idx < values.size() && depth < code_lengths.size()) {
            tree_.emplace_back(0, 0, 0);
            tree_[idx].left_ = tree_.size() - 1;
            MakeTree(tree_[idx].left_, depth + 1, code_lengths, values, values_idx);
        }
        if (values_idx < values.size() && depth < code_lengths.size()) {
            tree_.emplace_back(0, 0, 0);
            tree_[idx].right_ = tree_.size() - 1;
            MakeTree(tree_[idx].right_, depth + 1, code_lengths, values, values_idx);
        }
    }
};

HuffmanTree::HuffmanTree() = default;

void HuffmanTree::Build(const std::vector<uint8_t> &code_lengths,
                        const std::vector<uint8_t> &values) {
    impl_ = std::make_unique<Impl>(code_lengths, values);
}

bool HuffmanTree::Move(bool bit, int &value) {
    if (impl_.get() == nullptr) {
        DLOG(ERROR) << "Use uncomplete huffman tree\n";
        throw std::invalid_argument("");
    }
    return impl_->Move(bit, value);
}

HuffmanTree::HuffmanTree(HuffmanTree &&) = default;

HuffmanTree &HuffmanTree::operator=(HuffmanTree &&) = default;

HuffmanTree::~HuffmanTree() = default;
