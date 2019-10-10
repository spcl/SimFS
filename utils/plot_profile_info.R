#!/usr/bin/env Rscript
args = commandArgs(trailingOnly=TRUE)

#setwd("/home/salvo/ETH/SimFS/utils/")

source("utils.R")
source("stats.R")
source("aes.R")

library(ggplot2)
library(plyr)

if (length(args)!=2) {
  stop("Need to specify the profiling file and the output pdf location!");
}

prof_file = args[1];
pdf_file = args[2];


profile_data <- ReadFile(file.path=prof_file, col=c("op_name", "op_count", "op_id", "id", "time", "overhead"), pheader=TRUE)


plot <- ggplot(data=profile_data, aes(x=time/1000, y=op_count)) +
  geom_line() + 
  geom_point(aes(color=op_name, shape=op_name), size = 3) + 
  theme_bw() + 
  scale_x_continuous("Time (ms)") + 
  scale_y_continuous("Operation count") +
  scale_color_discrete(name="Operation type") +
  scale_shape_discrete(name="Operation type") 
plot

PrintGGPlotOnPDF(plot, pdf_file, 10, 6)
