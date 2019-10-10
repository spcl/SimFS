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
profile_data.open <- subset(profile_data, op_name=="dvl_nc_open")

lbls <- c("nc_open_start", "nc_open_end")
plot <- ggplot(data=profile_data.open, aes(x=time/1000, y=op_count)) +
  geom_line() + 
  geom_point(aes(color=factor(id), shape=factor(id)), size = 3) + 
  theme_bw() + 
  scale_x_continuous("Time (ms)") + 
  scale_y_continuous("Operation count") +
  scale_color_discrete(name="Operation type", labels=lbls) +
  scale_shape_discrete(name="Operation type", labels=lbls) 
plot

PrintGGPlotOnPDF(plot, pdf_file, 10, 6)
