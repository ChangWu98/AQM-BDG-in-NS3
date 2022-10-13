# 基本命令
git init--创建git仓库(文件夹中新增.git文件夹)
创建文件/改动文件
git status--查看当前仓库状态
git add <filename>--将文件加入暂存区
git add . --将当前文件夹下所有文件加入暂存区

git commit -m "message"--提交(message:对此次提交的描述,git commit只提交add过的文件)
git log--查看日志

git reset <filename>--将文件移出暂存区

# Git中文件的四种状态：
Untracked(新建/从仓库remove)
Unmodified(已通过commit提交至仓库中)
Modified(仓库中文件经过更改)
Staged(通过add加入暂存区)

# 回退Reset
通过git log找到想回退的commitID
git reset <commitID>--将现有文件回退到commitID对应的文件状态
## 回到新版本
* git reflog查看所有操作记录，找到想回退的commitID；git reset <commitID>
* git pull--回到最新版本
## reset模式影响
* --hard：不保存所有变更
* --soft：保留变更且变更内容处于Staged
* --mixed：保留变更内容且变更内容处于Modified

# 创建新分支(取代回退，不需要查找commitID)
git checkout -b <name> <template>(name：新分支的名字；template:模板，默认当前分支为模板）
git checkout master--切换回主分支
git branch --查看所有分支(高亮的条目表示当前所处的分支)

# 合并merge
git merge <branchName> --合并分支的变更
## 例：在1.0.0分支上,小A、小B分别开发,主管C负责合并
* 小A：在1.0.0分支上通过git checkout -b bc-a创建”bc-a“分支；在此分支下进行工作，add、commit提交
* 小B：在1.0.0分支上通过git checkout -b bc-b创建”bc-b“分支；在此分支下进行工作，add、commit提交
* C主管：在1.0.0分支上，通过git merge bc-a合并小A的代码（此时内容更新），通过git merge bc-b合并小B的代码（内容再次更新），（产生冲突，解决冲突），add、commit提交

# 远程仓库Remote
远程仓库作为中央仓库管理所有分支



