clc;
close all;

data = out.run_data.Data;
t = out.run_data.Time;

target = data(: , 1);
actual = data(: , 2);

t_idx = find(actual > 0 , 1 , "first");
step_time = t(t_idx);

initial_target = target(1);
final_target = target(end);
step_height = final_target - initial_target;

peak_val = max(actual);
overshoot = max(0 , ((peak_val - final_target) / step_height) * 100);

t_10_idx = find(actual >= (initial_target + 0.1*step_height) , 1 , "first");
t_90_idx = find(actual >= (initial_target + 0.9*step_height) , 1 , "first");
rise_time = t(t_90_idx) - t(t_10_idx);

settle_percentage = 3;
settle_band = (settle_percentage/100)*final_target;
out_of_bounds = find(abs(actual - final_target) > settle_band);

if ~isempty(out_of_bounds)
settling_time = t(out_of_bounds(end)) - step_time;

else
    settling_time = 0;   
end

fprintf("Target Setpoint : %.2f RPM \n" , final_target)
fprintf("Settling Time : %.3f seconds \n", settling_time)
fprintf("Rise Time : %.3f seconds \n" , rise_time)
fprintf("Overshoot : %.2f %% \n" , overshoot)

figure("Color" , "k")
plot(t , target , "y-" , LineWidth=1.5);
hold on;
plot(t , actual , "r-" , LineWidth=2);

yline(final_target-settle_band , "w--" , LineWidth=1);
yline(final_target+settle_band , "w--" , LineWidth=1);

title(sprintf("\n Motor PI Controller Step Response Analysis \n"), "FontSize",16);
subtitle(sprintf("Rise Time: %.3f s  |  Settling Time: %.3f s  |  Overshoot: %.2f %%  |  Step: %.f  |  Settling Band: %.f%% \n" ...
, rise_time , settling_time , overshoot , final_target , settle_percentage) , "FontSize",12);

xlabel("Time (s)" , "Color","w");
ylabel("RPM" , "Color","w");


grid on;
legend("Target RPM" , "Actual RPM" , "Location","northwest");
