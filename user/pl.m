% Copyright David Rowe 2007 
% This program is distributed under the terms of the GNU General Public License 
% Version 2

function pl(samplename, start_sam, end_sam, pngname)
  
  ftx=fopen(strcat(samplename, "_tx.raw"),"rb");
  tx=fread(ftx,Inf,"short");
  frx=fopen(strcat(samplename, "_rx.raw"),"rb");
  rx=fread(frx,Inf,"short");
  fec=fopen(strcat(samplename, "_ec.raw"),"rb");
  ec=fread(fec,Inf,"short");

  st = 1;
  en = length(tx);
  if (nargin >= 2)
    st = start_sam;
  endif
  if (nargin >= 3)
    en = end_sam;
  endif

  figure(1);
  clg;
  plot(tx(st:en),"r;tx;", rx(st:en),"g;rx;", ec(st:en),"b;ec;");
  %axis([1 en-st min(tx) max(tx)]);
 
  if (nargin == 4)

    % small image

    __gnuplot_set__ terminal png size 420,300
    s = sprintf("__gnuplot_set__ output \"%s.png\"", pngname);
    eval(s)
    replot;

    % larger image

    __gnuplot_set__ terminal png size 800,600
    s = sprintf("__gnuplot_set__ output \"%s_large.png\"", pngname);
    eval(s)
    replot;

  endif

endfunction
