function [out] = MGTester()
close all

% default parameters
N = 129;
omega = 0.6;
Sweep = [3,3];

% Convergence for a V cycle
Nlist = [33,65,129];
[ cycleline, lgds ] = MGVCycle(Nlist,Sweep,omega);
figure
semilogy(cycleline);
legend(lgds);
title('Convergence for a V cycle');
xlabel('# V cycle')
ylabel('Inf norm of residuals')
grid on

% Vary the relaxation parameter
omegas = [0.1,0.3,0.5,0.7,0.9];
[ cycleline, lgds ] = MGRelaxation(N,Sweep,omegas);
figure
semilogy(cycleline);
legend(lgds);
title('Vary the relaxation parameter');
xlabel('# V cycle')
ylabel('Inf norm of residuals')
grid on

% Vary the sweep count
Sweeps = [1,2;2,1;3,3;5,5;7,7];
[ cycleline, lgds ] = MGSpeed(N,Sweeps,omega);
figure
semilogy(cycleline);
legend(lgds);
title('Vary the sweep count');
xlabel('# V cycle')
ylabel('Inf norm of residuals')
grid on

end

function [ lines, legends ] = MGVCycle(Nlist, Sweeps, omega)
len = length(Nlist);
lines = [];
legends = cell(1,len*2);
for i=1:len
    generateInput(Nlist(i),1);
    runMG(Sweeps(1), Sweeps(2), omega);
    load Output.mat
    lines = [lines, ResNorms];
    legends{i} = ['N=', num2str(Nlist(i)),' smooth rhs'];
end

for i=1:len
    generateInput(Nlist(i),0);
    runMG(Sweeps(1), Sweeps(2), omega);
    load Output.mat
    lines = [lines, ResNorms];
    legends{i+len} = ['N=', num2str(Nlist(i)), ' random rhs'];
end
end

function [ lines, legends ] = MGRelaxation(N,Sweeps,omega)
len = length(omega);
lines = [];
legends = cell(1,len);
for i=1:len
    generateInput(N,1);
    runMG(Sweeps(1), Sweeps(2), omega(i));
    load Output.mat
    lines = [lines, ResNorms];
    legends{i} = ['\omega=', num2str(omega(i))];
end
end

function [ lines, legends ] = MGSpeed(N,Sweeps,omega)
% TODO: should create two graphs, one where the x axis is scaled by the number of sweeps at the top level, and another with wall time
len = size(Sweeps, 1);
lines = [];
legends = cell(1,len);
for i=1:len
    generateInput(N,1);
    runMG(Sweeps(i,1), Sweeps(i,2), omega);
    load Output.mat
    lines = [lines, ResNorms];
    legends{i} = ['N_a=', num2str(Sweeps(i,1)), ' N_b=', num2str(Sweeps(i,2))];
end
end


function [ f ] = generateInput( N, smooth )
load('InputStd.mat')
f = rand(N, N);
f_loc(3) = 1 / (N-1);
f_loc(4) = f_loc(3);
if(smooth == 1)
    for i=1:N
        x = f_loc(1) + (i-1)*f_loc(3);
        for j=1:N
            y = f_loc(2) + (j-1)*f_loc(4);
            f(i,j) = (x^2 + y^2) * exp(x * y);
        end
    end
end
save Input.mat Seq_f f f_loc -v4
end


function [ cmd, status ] = runMG( Nb,Na,omega )
Nv = 1000;
cmd = ['build/Release/multigrid -v ', num2str(Nv), ' -b ', num2str(Nb), ' -a ', num2str(Na), ' -o ', num2str(omega)];
status = system(cmd);

end