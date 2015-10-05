" Vim syntax file
" Language: BNF grammar
" Maintainer: Alexey Galakhov
" Latest Revision: 05 October 2015

if exists("b:current_syntax")
  finish
endif

syn include @C syntax/c.vim
syn region bnfHostCode matchgroup=bnfHostCodeMatch start="{" end="}" contains=@C fold

syn match bnfDirective '%[a-z_]\+'
syn match bnfErrorProc '%syntax_error'
syn region bnfParen transparent start="(" end=")" contains=bnfVariable
syn region bnfDefinition matchgroup=bnfOperator start="::=" end="[.]" contains=bnfRightSide,bnfParen
syn match bnfVariable '[A-Za-z][A-Za-z0-9_]*' contained
syn match bnfLeftSide '[A-Za-z][A-Za-z0-9_]*'
syn match bnfRightSide '[A-Za-z][A-Za-z0-9_]*' contained

hi def link bnfOperator Operator
hi def link bnfDirective Statement
hi def link bnfHostCodeMatch Include
hi def link bnfVariable Variable
hi def link bnfLeftSide bnfName
hi def link bnfRightSide bnfName
hi def link bnfName Type
hi def link bnfErrorProc Exception
