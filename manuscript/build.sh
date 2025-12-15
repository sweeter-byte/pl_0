# 只删除 .aux 文件
rm main.aux

# 重新编译
xelatex main.tex
bibtex main
xelatex main.tex
xelatex main.tex