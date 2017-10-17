# png.R
# Converts ftq.dat files to PNG.
# 
# Handy bash one-liner for files names 40.dat, 41.dat, 42.dat:
# for i in `seq 40 42`; do Rscript ~/akaros/ftq/scripts/png.R -i $i.dat -o $i.png --ymin=650 --ymax=850; done

# library that includes the pwelch function
suppressPackageStartupMessages(library(oce))
# library for command line option parsing
suppressPackageStartupMessages(library(optparse))

### collect command line arguments
# establish optional arguments
# "-h" and "--help" are automatically in the list
option_list <- list(
	make_option(c("-i", "--input"), type="character", default="foo_0.dat"),
	make_option(c("-o", "--output"), type="character", default="foo.png"),
	make_option(c("-W", "--width"), type="double", default=2400),
	make_option(c("-H", "--height"), type="double", default=1200),
	make_option("--ymin", type="double", default=-1),
	make_option("--ymax", type="double", default=-1)
)

opt <- parse_args(OptionParser(option_list=option_list))
  
d <- read.table(opt$input)

if (opt$ymin != -1) {
	ymin = opt$ymin
} else {
	ymin = min(d$V2)
}
if (opt$ymax != -1) {
	ymax = opt$ymax
} else {
	ymax = max(d$V2)
}

png(opt$output, width=opt$width, height=opt$height)
plot(d, ylim = c(ymin, ymax))

invisible(dev.off())
