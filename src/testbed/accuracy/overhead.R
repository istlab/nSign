#Evaluates correlation between overhead time and source code length
#author circular, dimitro
#date 2010-10-21
#data file needs 2 columns, source input length (length) and overhead (time)

overhead <- read.table(file.choose(), header=T)
attach(overhead)

attributes(overhead)

lm.r <- lm(time ~ length)
 
summary(lm.r)
 
coef(lm.r)
 
layout(matrix(1:4,2,2))
 
plot(lm.r)
 
layout(matrix(1:1))

plot(time ~ length,
        xlab="length",
	    ylab="overhead time (microseconds)",
		pch=16
)
 
abline(lm.r,lty=1) 
title(main="Overhead Time vs Length")

anova(lm.r)