function [s, f] = dit(file, freq)
pkg load signal
data = load(file);
pwelch(data(:,2),[],[],[], freq, "half");
[s,f]=pwelch(data(:,2),[],[],[], freq, "half");
endfunction
