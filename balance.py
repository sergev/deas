#!/usr/local/bin/python

from deas import *
import string

register ()

#
# Вычисляем баланс счета.
#
def balance (acn):
	(acn, pacn, passive, anal, descr, deleted, rmask, wmask) = chart_info (acn)
	((dr, dd, dp, dn), (cr, cd, cp, cn)) = account_balance (acn)
	if passive:
		return (cr - dr, cd - dd)
	return (dr - cr, dd - cd)

#
# Форматируем рублевую сумму для печати.
#
def cvtr (v):
	return cvt (v, 0)

#
# Форматируем долларовую сумму для печати.
#
def cvtd (v):
	return cvt (v, 1)

#
# Форматируем сумму денег для печати:
# скобки для отрицательных величин, $ для долларов,
# прочерк для нуля, отделяем тысячи.
#
def cvt (v, d):
	if v == 0:
		return "             - "
	neg = 0
	if v < 0:
		neg = 1
		v = -v
	str = [' ']
	if neg:
		str[0] = ')'
	i = 0
	while v != 0:
		str[0:0] = ["0123456789" [v % 10]]
		v = v/10
		i = i+1
		if v and i % 3 == 0:
			str[0:0] = ["'"]
	if d:
		str[0:0] = ['$']
	if neg:
		str[0:0] = ['(']
	return string.rjust (string.joinfields (str, ""), 15)

#
# Печатаем баланс для конкретного подразделения.
#
def doit (name, inc, eqp, mat, exp, sal, cash, bank, doll):
	print "-" * 30, name, "-" * 30

	(in_rub, in_doll) = balance (inc)
	print "Доходы:\t\t\t\t", cvtr (in_rub), cvtd (in_doll)
	print

	eqp_rub = eqp_doll = mat_rub = mat_doll = 0
	if eqp:
		(eqp_rub, eqp_doll) = balance (eqp)
	if mat:
		(mat_rub, mat_doll) = balance (mat)
	(exp_rub, exp_doll) = balance (exp)
	(sal_rub, sal_doll) = balance (sal)
	print "Расходы:\t\t\t", cvtr (eqp_rub + mat_rub + exp_rub - sal_rub),\
		cvtd (eqp_doll + mat_doll + exp_doll - sal_doll)
	print "в том числе:\t\t\t", "-" * 32
	if eqp:
		print "\tЗакупка оборудования\t", cvtr (eqp_rub), cvtd (eqp_doll)
	if mat:
		print "\tЗакупка материалов\t", cvtr (mat_rub), cvtd (mat_doll)
	print "\tОбщие расходы\t\t", cvtr (exp_rub), cvtd (exp_doll)
	print "\tЗарплата\t\t", cvtr (-sal_rub), cvtd (-sal_doll)
	print

	doll_rub = doll_doll = 0;
	if doll:
		(doll_rub, doll_doll) = balance (doll)
	(cash_rub, cash_doll) = balance (cash)
	(bank_rub, bank_doll) = balance (bank)
	print "Финансы:\t\t\t", cvtr (cash_rub + bank_rub),\
		cvtd (cash_doll + doll_doll)
	print "в том числе:\t\t\t", "-" * 32
	print "\tКасса\t\t\t", cvtr (cash_rub), cvtd (cash_doll)
	print "\tРасчетный счет\t\t", cvtr (bank_rub)
	if doll:
		print "\tВалютный счет\t\t\t\t", cvtd (doll_doll)
	print

doit ("КБ",         146, 101, 110, 126, 170, 150, 151, 152)
