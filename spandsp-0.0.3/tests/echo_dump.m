% echo_dump.m
% David Rowe 23 Dec 2006
%
% Plots echo can dump file values

load dump.txt
load level.txt
st = 1;
en = length(dump);
%st=8000*0+1; en=8000*2;
tx = dump(st:en,1);
rx = dump(st:en,2);
ec = dump(st:en,4);
Ltx = dump(st:en,5);
Lrx = dump(st:en,6);
Lec = dump(st:en,7);
dtd = -9900 + 800*dump(st:en,8);
adapt = -9900 + 800*dump(st:en,9);
Lbgec = dump(st:en,10);

figure(1);
plot(tx,"r;tx;", rx,"g;rx;", ec,"b;ec;", dtd,"g;dtd;");
%plot(tx,"r;tx;", rx,"g;rx;", dtd,"w;dtd;");
axis([0 en-st -1E4 1E4]);

%__gnuplot_set__ terminal png size 450,338
%__gnuplot_set__ output "~/Desktop/fxo_oslec.png"
%__gnuplot_set__ terminal png size 800,600
%__gnuplot_set__ output "~/Desktop/fxo_oslec_large.png"
%replot;

LtxdB = 20*log10(Ltx+1);
LrxdB = 20*log10(Lrx+1);
LecdB = 20*log10(Lec+1);
LbgecdB = 20*log10(Lbgec+1);

figure(2);
plot(LtxdB,"r;LtxdB;", LrxdB,"g;LrxdB;", LtxdB-LrxdB,"w;LtxdB-LrxdB;", LtxdB-LecdB,"b;LtxdB-LecdB;", 80+5*dump(st:en,9), "w;adapt;", LtxdB-LbgecdB,"c;LtxdB-LbgecdB;", 90+5*dump(st:en,8), "r;dtd;");
axis([0 en-st 0 100]);
grid;

%__gnuplot_set__ terminal png size 800,600
%__gnuplot_set__ output "~/Desktop/levels.png"
%replot;

figure(3);
LRin = level(st:en,1);
LSin = level(st:en,2);
LSout = level(st:en,3);
LSgen = level(st:en,4);
axis([0 en-st -100 0]);
plot(LRin,"r;LRindB;", LSin,"g;LSindB;", LSout,"w;LSoutdB;", LSgen,"b;LSgendB;");

figure(4)
plot(dump(st:en,11), "r;Pstates;")
axis
figure(5)
plot(dump(st:en,12), "g;factor;")
figure(6)
plot(dump(st:en,13), "g;shift;")
