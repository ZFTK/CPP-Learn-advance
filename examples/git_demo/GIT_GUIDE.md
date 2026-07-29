# Git 协作速成指南

> 入职第一天就要用的 Git 命令

## 一、基本工作流（单人开发）

```bash
# 1. 克隆仓库
git clone https://github.com/company/project.git
cd project

# 2. 创建功能分支
git checkout -b feature/my-feature

# 3. 日常开发循环
git add src/new_file.cpp          # 暂存更改
git commit -m "feat: 添加新功能"   # 提交
git push origin feature/my-feature # 推送

# 4. 创建 Pull Request (PR) 进行代码评审
# (在 GitHub/GitLab 网页上操作)

# 5. 合并后更新本地
git checkout main
git pull origin main
```

## 二、分支策略

```
main / master     ← 生产就绪代码，只通过 PR/MR 合并
  ├── develop     ← 开发主线（有的团队不用）
  ├── feature/*   ← 功能开发，从 main 切出，合并回 main
  ├── bugfix/*    ← Bug 修复
  └── hotfix/*    ← 紧急修复，从 main 切出，合并回 main + develop
```

## 三、合并 vs Rebase

| 操作 | 效果 | 何时使用 |
|------|------|---------|
| `git merge` | 创建合并提交，保留完整历史 | 合并功能分支到主分支 |
| `git rebase` | 重写历史，线性化提交 | 整理个人分支前提交 |

```bash
# 合并：保留分叉历史
git checkout main
git merge feature/my-feature

# Rebase：线性历史（仅用于个人分支！）
git checkout feature/my-feature
git rebase main          # 将 feature 的提交"重放"到 main 最新处

# ⚠️ 黄金法则：永远不要 rebase 已推送的公共分支！
```

## 四、解决冲突

```bash
# 场景：merge 时遇到冲突
git merge feature/other
# CONFLICT in src/file.cpp

# 1. 查看冲突文件
git status

# 2. 手动编辑冲突文件，删除 <<<<<<< ======= >>>>>>> 标记

# 3. 标记已解决并提交
git add src/file.cpp
git commit -m "merge: 解决与 feature/other 的冲突"

# 4. 中止合并（如果不想解决了）
git merge --abort
```

## 五、常用协作命令

```bash
# 查看分支
git branch -a                    # 所有分支（含远程）
git branch -vv                   # 本地分支跟踪信息

# 日志查看
git log --oneline --graph --all  # 图形化提交历史
git log -p src/file.cpp          # 查看文件的修改历史
git blame src/file.cpp           # 逐行查看谁写的

# 暂存工作区
git stash                        # 暂存当前修改
git stash pop                    # 恢复暂存的修改
git stash list                   # 查看暂存列表

# 撤销操作
git reset HEAD~1                 # 撤销最近一次提交（保留修改）
git reset --hard HEAD~1          # 彻底撤销最近一次提交
git checkout -- src/file.cpp     # 丢弃文件修改

# 标签（版本发布）
git tag v1.0.0                   # 打轻量标签
git tag -a v1.0.0 -m "发布1.0"   # 打注释标签
git push origin v1.0.0           # 推送标签
```

## 六、.gitignore 模板

```
# 编译产物
build/
cmake-build-*/
*.o
*.obj
*.exe
*.dll
*.so
*.a

# IDE
.vscode/
.idea/
*.user
*.suo

# 依赖
node_modules/
vendor/

# 环境文件
.env
*.local

# 数据库
*.db
*.sqlite

# 临时文件
*.log
*.tmp
.DS_Store
Thumbs.db
```

## 七、提交信息规范

```
<type>: <简短描述>              # 标题（50字符以内）

<详细描述>                        # 正文（可选，72字符换行）

<关联Issue>                       # 页脚（可选）
```

类型: `feat` (新功能), `fix` (修复), `docs` (文档), `refactor` (重构),
      `test` (测试), `chore` (构建/工具), `style` (格式)

示例:
```
feat: 添加用户登录API

实现了基于JWT的用户认证流程，包括登录接口和token刷新。

Closes #123
```

## 八、常见场景速查

```bash
# 场景1: 忘记切分支就开始写了
git stash
git checkout -b feature/new
git stash pop

# 场景2: 提交信息写错了
git commit --amend -m "新的提交信息"

# 场景3: 漏了文件
git add forgotten.cpp
git commit --amend --no-edit

# 场景4: 撤销未推送的提交
git reset --soft HEAD~1    # 保留修改在暂存区
git reset --mixed HEAD~1   # 保留修改在工作区（默认）

# 场景5: 从远程获取最新并变基
git pull --rebase origin main

# 场景6: cherry-pick 某次提交到当前分支
git cherry-pick <commit-hash>
```
