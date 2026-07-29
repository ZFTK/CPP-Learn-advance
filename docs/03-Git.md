# 03 — Git 协作工作流实战

## 1. 核心概念

Git 是分布式版本控制系统，记录的是文件快照而非差异。核心模型：

- **工作区**（Working Directory）→ `git add` → **暂存区**（Staging）→ `git commit` → **本地仓库**（Local Repo）→ `git push` → **远程仓库**（Remote）

**为什么用Git？**团队协作、版本回溯、分支隔离开发、代码审查，是现代软件工程的必备技能。

---

## 2. 快速上手

```bash
# 初始化 / 克隆
git init                              # 新仓库
git clone https://github.com/user/repo.git

# 日常工作流
git status                            # 查看状态
git add main.cpp                      # 暂存特定文件
git add .                             # 暂存所有变更
git commit -m "feat: 添加登录功能"     # 提交
git push origin main                  # 推送到远程

# 拉取更新
git pull origin main                  # = fetch + merge
git pull --rebase origin main         # = fetch + rebase（推荐）
```

### 提交信息规范（Conventional Commits）

```
feat: 新功能
fix: 修复bug
docs: 文档变更
refactor: 重构（不改变功能）
test: 测试相关
chore: 构建/工具变更
```

示例：`feat(auth): 添加JWT令牌验证` `fix(ui): 修复按钮点击无响应`

---

## 3. 核心操作速查

### 3.1 分支操作

```bash
git branch                    # 列出本地分支
git branch -a                 # 列出所有分支（含远程）
git branch feature-login      # 创建分支
git switch feature-login      # 切换分支（新命令，推荐）
git checkout feature-login    # 切换分支（旧命令）
git switch -c feature-pay     # 创建并切换
git branch -d feature-login   # 删除已合并的分支
git branch -D feature-login   # 强制删除
```

### 3.2 合并策略：merge vs rebase

```bash
# === merge：保留完整历史 ===
git switch main
git merge feature-login       # 产生一个合并提交

# === rebase：线性历史，更整洁 ===
git switch feature-login
git rebase main               # 把feature的提交"搬到"main最新提交之后
git switch main
git merge feature-login       # 此时是 fast-forward，无额外合并提交
```

**选择：**个人分支用 rebase 保持整洁；公共分支用 merge 保留痕迹。

### 3.3 解决冲突

```bash
# 冲突发生时：
# <<<<<<< HEAD         — 当前分支
# int x = 10;
# =======
# int x = 20;         — 合并进来的分支
# >>>>>>> feature-login

# 手动编辑，保留正确版本，然后：
git add conflicted_file.cpp
git commit                      # Git会自动生成合并提交信息

# 放弃合并
git merge --abort
# 放弃rebase
git rebase --abort
```

### 3.4 历史操作

```bash
git log --oneline --graph --all    # 可视化历史
git log -3                         # 最近3条
git diff                           # 工作区 vs 暂存区
git diff --staged                  # 暂存区 vs 最新提交
git diff main..feature             # 两个分支的差异

git stash                          # 暂存当前修改
git stash pop                      # 恢复最近暂存

# 撤销
git reset --soft HEAD~1            # 撤销commit，保留修改在暂存区
git reset --hard HEAD~1            # 彻底丢弃最近1次commit
git restore main.cpp               # 丢弃工作区修改（新命令）
git checkout -- main.cpp           # 同上（旧命令）
```

### 3.5 .gitignore

```gitignore
# 编译产物
build/
*.o
*.obj
*.exe
*.out

# IDE
.vs/
.vscode/
*.user
.idea/

# 系统文件
.DS_Store
Thumbs.db

# 依赖
node_modules/
```

### 3.6 子模块（submodule）

```bash
# 添加子模块
git submodule add https://github.com/libs/json.git third_party/json

# 克隆含子模块的仓库
git clone --recurse-submodules <url>

# 已克隆但缺子模块时
git submodule update --init --recursive

# 更新子模块到远程最新
git submodule update --remote
```

---

## 4. Git Flow 分支策略

```
main          ●──────●────────●─────── 生产发布
               \    /        /
release        ●──●        /          发布分支
                   \      /
develop     ●──●───●────●───●─────    开发主线
             \    /    /
feature-a     ●──●    /              功能分支
                     /
feature-b      ●────●                 功能分支
```

### 分支命名与生命周期

| 分支 | 来源 | 合并到 | 命名 |
|------|------|--------|------|
| `main` | - | - | main / master |
| `develop` | main | main | develop |
| `feature/*` | develop | develop | feature/login |
| `release/*` | develop | main + develop | release/1.2.0 |
| `hotfix/*` | main | main + develop | hotfix/urgent-fix |

### 简化版：GitHub Flow（推荐小团队）

只有 `main` + `feature/*` 分支，功能分支通过 PR 合并。

---

## 5. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **提交大文件** | 二进制文件让仓库迅速膨胀 | 加入`.gitignore`，大文件用Git LFS |
| **在main上直接开发** | 失去分支隔离 | 始终在功能分支上工作 |
| **force push到共享分支** | 覆盖他人提交 | 仅对个人分支使用`--force-with-lease` |
| **提交敏感信息** | 密码/密钥入库 | `.gitignore`排除配置文件模板，真密码用环境变量 |
| **merge commit地狱** | 频繁互相merge导致历史混乱 | 团队统一策略：merge或rebase二选一 |
| **pull不指定策略** | 默认merge产生多余合并提交 | 设置`git config --global pull.rebase true` |

**最佳实践速记：**

- 一个分支只做一件事，提交粒度要小
- 推送前先在本地 rebase 最新main，解决冲突
- 提交信息用中文或英文都可以，但要统一格式
- 定期 `git fetch --prune` 清理已删除的远程分支引用

---

## 6. 协作场景演练

### 场景：Code Review 流程

```bash
# 1. 从最新的main创建功能分支
git switch main
git pull --rebase origin main
git switch -c feature/api-cache

# 2. 开发并提交
# ... 写代码 ...
git add src/cache.cpp src/cache.h
git commit -m "feat(cache): 添加API响应缓存层"

# 3. rebase到最新main（解决冲突）
git fetch origin
git rebase origin/main

# 4. 推送并创建PR
git push origin feature/api-cache
# 在GitHub/GitLab上创建Pull Request / Merge Request

# 5. Review修改后
git add .
git commit -m "fix: 按review意见修正缓存过期逻辑"
git push origin feature/api-cache

# 6. 合并后清理
git switch main
git pull --rebase origin main
git branch -d feature/api-cache
```

### 场景：Hotfix紧急修复

```bash
git switch main
git pull --rebase origin main
git switch -c hotfix/crash-on-null

# 修复bug，提交
git add src/parser.cpp
git commit -m "fix(parser): 修复空指针崩溃"

# 合并到main
git switch main
git merge --no-ff hotfix/crash-on-null   # --no-ff 保留分支痕迹
git push origin main

# 同步到develop
git switch develop
git merge --no-ff hotfix/crash-on-null
git push origin develop

# 打标签
git tag -a v1.2.1 -m "hotfix: 修复空指针崩溃"
git push origin v1.2.1
```

---

## 参考链接

- [Pro Git 中文版](https://git-scm.com/book/zh/v2)
- [Conventional Commits](https://www.conventionalcommits.org/zh-hans/)
- [Learn Git Branching（交互式学习）](https://learngitbranching.js.org/)
