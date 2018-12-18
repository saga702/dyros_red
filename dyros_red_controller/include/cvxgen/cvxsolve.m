% Produced by CVXGEN, 2018-12-11 02:48:56 -0500.
% CVXGEN is Copyright (C) 2006-2017 Jacob Mattingley, jem@cvxgen.com.
% The code in this file is Copyright (C) 2006-2017 Jacob Mattingley.
% CVXGEN, or solvers produced by CVXGEN, cannot be used for commercial
% applications without prior written permission from Jacob Mattingley.

% Filename: cvxsolve.m.
% Description: Solution file, via cvx, for use with sample.m.
function [vars, status] = cvxsolve(params, settings)
H = params.H;
f = params.f;
g = params.g;
r = params.r;
cvx_begin
  % Caution: automatically generated by cvxgen. May be incorrect.
  variable P(60, 1);

  minimize(0.5*quad_form(P, H) + g'*P);
  subject to
    -f <= P(1) - r(1);
    -f <= P(2) - r(2);
    -f <= P(3) - r(3);
    -f <= P(4) - r(4);
    -f <= P(5) - r(5);
    -f <= P(6) - r(6);
    -f <= P(7) - r(7);
    -f <= P(8) - r(8);
    -f <= P(9) - r(9);
    -f <= P(10) - r(10);
    -f <= P(11) - r(11);
    -f <= P(12) - r(12);
    -f <= P(13) - r(13);
    -f <= P(14) - r(14);
    -f <= P(15) - r(15);
    -f <= P(16) - r(16);
    -f <= P(17) - r(17);
    -f <= P(18) - r(18);
    -f <= P(19) - r(19);
    -f <= P(20) - r(20);
    -f <= P(21) - r(21);
    -f <= P(22) - r(22);
    -f <= P(23) - r(23);
    -f <= P(24) - r(24);
    -f <= P(25) - r(25);
    -f <= P(26) - r(26);
    -f <= P(27) - r(27);
    -f <= P(28) - r(28);
    -f <= P(29) - r(29);
    -f <= P(30) - r(30);
    -f <= P(31) - r(31);
    -f <= P(32) - r(32);
    -f <= P(33) - r(33);
    -f <= P(34) - r(34);
    -f <= P(35) - r(35);
    -f <= P(36) - r(36);
    -f <= P(37) - r(37);
    -f <= P(38) - r(38);
    -f <= P(39) - r(39);
    -f <= P(40) - r(40);
    -f <= P(41) - r(41);
    -f <= P(42) - r(42);
    -f <= P(43) - r(43);
    -f <= P(44) - r(44);
    -f <= P(45) - r(45);
    -f <= P(46) - r(46);
    -f <= P(47) - r(47);
    -f <= P(48) - r(48);
    -f <= P(49) - r(49);
    -f <= P(50) - r(50);
    -f <= P(51) - r(51);
    -f <= P(52) - r(52);
    -f <= P(53) - r(53);
    -f <= P(54) - r(54);
    -f <= P(55) - r(55);
    -f <= P(56) - r(56);
    -f <= P(57) - r(57);
    -f <= P(58) - r(58);
    -f <= P(59) - r(59);
    -f <= P(60) - r(60);
    P(1) - r(1) <= f;
    P(2) - r(2) <= f;
    P(3) - r(3) <= f;
    P(4) - r(4) <= f;
    P(5) - r(5) <= f;
    P(6) - r(6) <= f;
    P(7) - r(7) <= f;
    P(8) - r(8) <= f;
    P(9) - r(9) <= f;
    P(10) - r(10) <= f;
    P(11) - r(11) <= f;
    P(12) - r(12) <= f;
    P(13) - r(13) <= f;
    P(14) - r(14) <= f;
    P(15) - r(15) <= f;
    P(16) - r(16) <= f;
    P(17) - r(17) <= f;
    P(18) - r(18) <= f;
    P(19) - r(19) <= f;
    P(20) - r(20) <= f;
    P(21) - r(21) <= f;
    P(22) - r(22) <= f;
    P(23) - r(23) <= f;
    P(24) - r(24) <= f;
    P(25) - r(25) <= f;
    P(26) - r(26) <= f;
    P(27) - r(27) <= f;
    P(28) - r(28) <= f;
    P(29) - r(29) <= f;
    P(30) - r(30) <= f;
    P(31) - r(31) <= f;
    P(32) - r(32) <= f;
    P(33) - r(33) <= f;
    P(34) - r(34) <= f;
    P(35) - r(35) <= f;
    P(36) - r(36) <= f;
    P(37) - r(37) <= f;
    P(38) - r(38) <= f;
    P(39) - r(39) <= f;
    P(40) - r(40) <= f;
    P(41) - r(41) <= f;
    P(42) - r(42) <= f;
    P(43) - r(43) <= f;
    P(44) - r(44) <= f;
    P(45) - r(45) <= f;
    P(46) - r(46) <= f;
    P(47) - r(47) <= f;
    P(48) - r(48) <= f;
    P(49) - r(49) <= f;
    P(50) - r(50) <= f;
    P(51) - r(51) <= f;
    P(52) - r(52) <= f;
    P(53) - r(53) <= f;
    P(54) - r(54) <= f;
    P(55) - r(55) <= f;
    P(56) - r(56) <= f;
    P(57) - r(57) <= f;
    P(58) - r(58) <= f;
    P(59) - r(59) <= f;
    P(60) - r(60) <= f;
cvx_end
vars.P = P;
status.cvx_status = cvx_status;
% Provide a drop-in replacement for csolve.
status.optval = cvx_optval;
status.converged = strcmp(cvx_status, 'Solved');