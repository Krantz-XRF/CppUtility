#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <iomanip>

template <typename T> class zipper;

// 节点类型
template <typename T>
class node
{
protected:
    // 建立多叉树
    std::vector<node<T>> children;
    node<T>* parent{nullptr};
    // 存储节点的数据
    T value;
public:
    node() noexcept = default;
    node(node* parent, const T& value) noexcept
        : parent{parent}, value{value} { }
    node(node* parent, T&& value) noexcept
        : parent{parent}, value{std::move_if_noexcept(value)} { }
    template <typename ... Args>
    node(node* parent, Args&& ... args) noexcept
        : parent{parent}, value{std::forward<Args>(args) ...} { }

    // 添加新的分支
    template <typename ... Args>
    void emplace_branch(Args&& ... args)
    {
        children.emplace_back(this, std::forward<Args>(args) ...);
    }

    // 判断符合条件的分支是否存在
    template <typename Predict>
    int find_branch(Predict p) const
    {
        auto pos = std::find_if(
            children.begin(),
            children.end(),
            [p](const node<T>& n){ p(n.value); }
        );
        if (pos == children.end())
            return -1;
        else
            return pos - children.begin();
    }

    // 辅助别名，解决std::vector<void>的错误
    template <typename U>
    using vector_of = std::conditional_t
        <std::is_same_v<U, void>, void, std::vector<U>>;
    // 对所有儿子应用函数p，将所有返回值依次存放在std::vector中返回
    template <typename Processor>
    auto map_children(Processor p) -> vector_of<decltype(p(value))>
    {
        if constexpr (std::is_same_v<decltype(p(children[0])), void>)
        {
            foreach_child(p);
        }
        else
        {
            std::vector<decltype(p(value))> result;
            foreach_child([&result, p](node& n){
                result.push_back(p(n));
            });
            return result;
        }
    }
    // 对所有儿子应用函数p，忽略所有返回值
    template <typename Processor>
    void foreach_child(Processor p)
    {
        std::for_each(children.begin(), children.end(), [p](node<T>& n){ p(n.value); });
    }
    // 将所有儿子的列表折叠成一个值
    // 类型约束：
    // Folder ~= Node -> Acc -> Acc
    // RetType ~= Acc
    template <typename Folder, typename Acc>
    auto fold_children(Folder f, Acc init = Acc()) -> Acc
    {
        using RetType = decltype(f(children[0], init));
        static_assert(std::is_convertible_v<Acc, RetType>, "fold: type not compatible.");
        static_assert(std::is_convertible_v<RetType, Acc>, "fold: type not compatible.");

        RetType res = static_cast<RetType>(init);
        
        for (auto&& v : children)
            res = f(v.value, res);

        return static_cast<Acc>(res);
    }

    // 调试方法，打印树状图
    void printTree(std::ostream& output, int depth = 0) const
    {
        output << std::setw(depth * 4) << value << std::endl;
        for (auto&& ch : children)
            ch.printTree(output, depth + 1);
    }

    friend class zipper<T>;
};

template <typename T>
class zipper
{
protected:
    node<T> root;
    node<T>* current{&root};
public:
    // 默认构造函数
    zipper() = default;
    // 初始化根节点
    template <typename ... U>
    zipper(U&& ... args) : root{nullptr, std::forward<U>(args) ...} { }

    // 后退相关的函数
    [[gnu::always_inline, gnu::pure]]
    bool can_step_back() const { return current->parent != nullptr; }
    void step_back() noexcept(false)
    {
        if (! can_step_back())
            throw std::runtime_error("zipper::step_back: cannot step back.");
        current = current->parent;
    }
    void step_back(int n) noexcept(false)
    {
        for (int i = 0; i < n; ++ i)
            step_back();
    }
    // 前进相关的函数
    [[gnu::always_inline, gnu::pure]]
    bool can_step_forward() const { return !current->children.empty(); }
    template <typename Predict>
    [[gnu::always_inline, gnu::pure]]
    int find_branch(Predict p) const { current->find_branch(p); }
    void step_forward(int branch)
    {
        if (branch < 0)
            throw std::runtime_error(
                "zipper::step_forward: invalid branch id\n"
                "\tpossibly did not check the result of zipper::find_branch."
            );
        if (branch >= current->children.size())
            throw std::runtime_error(
                "zipper::step_forward: invalid branch id\n"
                "\tuse only the result of zipper::find_branch."
            );
        current = &current->children[branch];
    }
    // 创建新的分支
    template <typename ... Args>
    void entre_new_branch(Args&& ... args)
    {
        current->emplace_branch(std::forward<Args>(args) ...);
        current = &current->children.back();
    }
    // 检查当前的分支选择
    [[gnu::always_inline]]
    std::vector<node<T>>& available_branches() const { return current->children; }

    // 对所有分支应用函数p，将所有返回值依次存放在std::vector中返回
    template <typename Processor>
    [[gnu::always_inline]]
    auto map_branches(Processor p)
    {
        using RetType = decltype(current->map_children(p));
        if constexpr (std::is_same_v<RetType, void>)
            current->map_children(p);
        else
            return current->map_children(p);
    }
    // 对所有分支应用函数p，忽略所有返回值
    template <typename Processor>
    [[gnu::always_inline]]
    void foreach_branch(Processor p) { current->foreach_branch(p); }
    // 将所有分支的列表折叠成一个值
    // 类型约束：
    // Folder ~= Node -> Acc -> Acc
    // RetType ~= Acc
    template <typename Folder, typename Acc>
    [[gnu::always_inline]]
    auto fold_children(Folder f, Acc&& init = std::decay_t<Acc>()) { return current->fold_children(f, init); }

    // 打印树状图
    [[gnu::always_inline]]
    void printTree(std::ostream& output) const { root.printTree(output, 0); }
};
