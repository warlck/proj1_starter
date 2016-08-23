# CS 61C Spring 2015 Project 1-2 
# string.s

#==============================================================================
#                              Project 1-2 Part 1
#                               String README
#==============================================================================
# In this file you will be implementing some utilities for manipulating strings.
# The functions you need to implement are:
#  - strlen()
#  - strncpy()
#  - copy_of_str()
# Test cases are in linker-tests/test_string.s
#==============================================================================

.data
newline:	.asciiz "\n"
tab:	.asciiz "\t"

.text
#------------------------------------------------------------------------------
# function strlen()
#------------------------------------------------------------------------------
# Arguments:
#  $a0 = string input
#
# Returns: the length of the string
#------------------------------------------------------------------------------
strlen:
	li $t0, 0
strlen_loop:
	lb $t1, 0($a0)
	beq $t1, $0, end_of_string
	addiu $t0, $t0, 1
	addiu $a0, $a0, 1
	j strlen_loop
end_of_string:
	addiu $v0, $t0, 0
	jr $ra

#------------------------------------------------------------------------------
# function strncpy()
#------------------------------------------------------------------------------
# Arguments:
#  $a0 = pointer to destination array
#  $a1 = source string
#  $a2 = number of characters to copy
#
# Returns: the destination array
#------------------------------------------------------------------------------
strncpy:
	lb $t4, 0($a1)
	addiu $t1, $a0, 0
	addiu $t2, $a1, 0
strncpy_loop:
	beq $a2, $0, end_of_count
	bne $t4, $0, load_char
	addiu $t0, $0, 0
	j store_char
load_char:
	lb $t0, 0($t2)
store_char:
	sb $t0, 0($t1) 
	addiu $t4, $t0, 0
	addiu $t2, $t2, 1
	addiu $t1, $t1, 1
	addiu $a2, $a2, -1
	j strncpy_loop
end_of_count:
	addiu $v0, $a0, 0
	jr $ra

#------------------------------------------------------------------------------
# function copy_of_str()
#------------------------------------------------------------------------------
# Creates a copy of a string. You will need to use sbrk (syscall 9) to allocate
# space for the string. strlen() and strncpy() will be helpful for this function.
# In MARS, to malloc memory use the sbrk syscall (syscall 9). See help for details.
#
# Arguments:
#   $a0 = string to copy
#
# Returns: pointer to the copy of the string
#------------------------------------------------------------------------------
copy_of_str:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $s1, 0($sp)

	addiu $s1, $a0, 0  # store the contents of argument for later recovery

	jal strlen
	addiu $t0, $v0, 0  # stores the the lenght of string in $t0
	addiu $t0, $t0, 1  # increment the len of string by 1 for null character

	addiu $a0, $t0, 0  # set the $a0 value to num of bytes that need to be allocated
	li $v0, 9
	syscall  # $v0 contains the address of allocated space

	addiu $a0, $v0, 0 # $a0 is allocated space address
	addiu $a1, $s1, 0 # $a1 address of the string to copy
	addiu $a2, $t0, 0 # $a2 is the number of characters to copy
	jal strncpy # $v0 is address of copied string
	lw $ra, 4($sp)
	lw $s1, 0($sp)
	addiu $sp, $sp, 8

	jr $ra

###############################################################################
#                 DO NOT MODIFY ANYTHING BELOW THIS POINT                       
###############################################################################

#------------------------------------------------------------------------------
# function streq() - DO NOT MODIFY THIS FUNCTION
#------------------------------------------------------------------------------
# Arguments:
#  $a0 = string 1
#  $a1 = string 2
#
# Returns: 0 if string 1 and string 2 are equal, -1 if they are not equal
#------------------------------------------------------------------------------
streq:
	beq $a0, $0, streq_false	# Begin streq()
	beq $a1, $0, streq_false
streq_loop:
	lb $t0, 0($a0)
	lb $t1, 0($a1)
	addiu $a0, $a0, 1
	addiu $a1, $a1, 1
	bne $t0, $t1, streq_false
	beq $t0, $0, streq_true
	j streq_loop
streq_true:
	li $v0, 0
	jr $ra
streq_false:
	li $v0, -1
	jr $ra			# End streq()

#------------------------------------------------------------------------------
# function dec_to_str() - DO NOT MODIFY THIS FUNCTION
#------------------------------------------------------------------------------
# Convert a number to its unsigned decimal integer string representation, eg.
# 35 => "35", 1024 => "1024". 
#
# Arguments:
#  $a0 = int to write
#  $a1 = character buffer to write into
#
# Returns: the number of digits written
#------------------------------------------------------------------------------
dec_to_str:
	li $t0, 10			# Begin dec_to_str()
	li $v0, 0
dec_to_str_largest_divisor:
	div $a0, $t0
	mflo $t1		# Quotient
	beq $t1, $0, dec_to_str_next
	mul $t0, $t0, 10
	j dec_to_str_largest_divisor
dec_to_str_next:
	mfhi $t2		# Remainder
dec_to_str_write:
	div $t0, $t0, 10	# Largest divisible amount
	div $t2, $t0
	mflo $t3		# extract digit to write
	addiu $t3, $t3, 48	# convert num -> ASCII
	sb $t3, 0($a1)
	addiu $a1, $a1, 1
	addiu $v0, $v0, 1
	mfhi $t2		# setup for next round
	bne $t2, $0, dec_to_str_write
	jr $ra			# End dec_to_str()
