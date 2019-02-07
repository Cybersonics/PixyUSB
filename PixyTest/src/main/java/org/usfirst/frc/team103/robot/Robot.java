package org.usfirst.frc.team103.robot;

import org.usfirst.frc103.Swerve2017Test.pixy.Pixy;
import org.usfirst.frc103.Swerve2017Test.pixy.Pixy.ExposureSetting;
import org.usfirst.frc103.Swerve2017Test.pixy.Pixy.WhiteBalanceSetting;

import edu.wpi.first.wpilibj.TimedRobot;
import edu.wpi.first.wpilibj.command.InstantCommand;
import edu.wpi.first.wpilibj.command.Scheduler;
import edu.wpi.first.wpilibj.smartdashboard.SmartDashboard;

public class Robot extends TimedRobot {
	Pixy pixy, pixy2;
	
	@Override
	public void robotInit() {

		//Pixy.ensureAvailable(0xC6E0B552);
		//pixy = new Pixy(0xC6E0B552);
		Pixy.ensureAvailable(0xDC8D360A, 0x53C3165);
        pixy = new Pixy(0xDC8D360A);
        pixy.startBlockProgram();
		pixy.startFrameGrabber();
		
		//Pixy.ensureAvailable(0x53C3165);
        pixy2 = new Pixy(0x53C3165);
        pixy2.startBlockProgram();
        pixy2.startFrameGrabber();
        
        SmartDashboard.putData("Enumerate", new InstantCommand() {
        	{
        		setRunWhenDisabled(true);
        	}
			@Override
			protected void initialize() {
				Pixy.enumerate();
			}
		});
        SmartDashboard.putData("GetParameters", new InstantCommand() {
        	{
        		setRunWhenDisabled(true);
        	}
        	@Override
        	protected void initialize() {
        		pixy.stopBlockProgram();
				pixy.stopFrameGrabber();

				pixy2.stopBlockProgram();
				pixy2.stopFrameGrabber();
				
				getParameters();
				
        		pixy.startBlockProgram();
				pixy.startFrameGrabber();
				
				//getParameters();
        		pixy2.startBlockProgram();
        		pixy2.startFrameGrabber();
        	}
        });
        SmartDashboard.putData("SetParameters", new InstantCommand() {
        	{
        		setRunWhenDisabled(true);
        	}
        	@Override
        	protected void initialize() {
        		pixy.stopBlockProgram();
				pixy.stopFrameGrabber();
				
				pixy2.stopBlockProgram();
				pixy2.stopFrameGrabber();
				
				setParameters();
				
        		//getParameters();
        		pixy.startBlockProgram();
				pixy.startFrameGrabber();
				
				//setParameters();
        		//getParameters();
        		pixy2.startBlockProgram();
        		pixy2.startFrameGrabber();
        	}
        });
	}
	
	@Override
	public void robotPeriodic() {
		Scheduler.getInstance().run();
	}
	
	private void getParameters() {
		boolean aec = pixy.getAutoExposure();
		boolean awb = pixy.getAutoWhiteBalance();
		ExposureSetting exp = pixy.getExposureCompensation();
		WhiteBalanceSetting wbv = pixy.getWhiteBalanceValue();
		SmartDashboard.putBoolean("AutoExposure", aec);
		SmartDashboard.putBoolean("AutoWhiteBalance", awb);
		SmartDashboard.putNumber("ExposureGain", exp.gain);
		SmartDashboard.putNumber("ExposureCompensation", exp.compensation);
		SmartDashboard.putNumber("WhiteBalanceRed", wbv.red);
		SmartDashboard.putNumber("WhiteBalanceGreen", wbv.green);
		SmartDashboard.putNumber("WhiteBalanceBlue", wbv.blue);

		
		boolean aec2 = pixy2.getAutoExposure();
		boolean awb2 = pixy2.getAutoWhiteBalance();
		ExposureSetting exp2 = pixy2.getExposureCompensation();
		WhiteBalanceSetting wbv2 = pixy2.getWhiteBalanceValue();
		SmartDashboard.putBoolean("02AuE2", aec2);
		SmartDashboard.putBoolean("02AuWht2", awb2);
		SmartDashboard.putNumber("02ExpGn2", exp2.gain);
		SmartDashboard.putNumber("02ExpComp2", exp2.compensation);
		SmartDashboard.putNumber("02WheBalRed2", wbv2.red);
		SmartDashboard.putNumber("02WhtBalGrn2", wbv2.green);
		SmartDashboard.putNumber("02WhtBalBlu2", wbv2.blue);
		
	}

	private void setParameters() {
		boolean aec = SmartDashboard.getBoolean("AutoExposure", false);
		boolean awb = SmartDashboard.getBoolean("AutoWhiteBalance", false);
		int gain = (int) SmartDashboard.getNumber("ExposureGain", 20);
		int comp = (int) SmartDashboard.getNumber("ExposureCompensation", 100);
		int r = (int) SmartDashboard.getNumber("WhiteBalanceRed", 64);
		int g = (int) SmartDashboard.getNumber("WhiteBalanceGreen", 64);
		int b = (int) SmartDashboard.getNumber("WhiteBalanceBlue", 64);
		pixy.setAutoExposure(aec);
		if (!aec) {
			pixy.setExposureCompensation(new ExposureSetting(gain, comp));
		}
		pixy.setAutoWhiteBalance(awb);
		if (!awb) {
			pixy.setWhiteBalanceValue(new WhiteBalanceSetting(r, g, b));
		}

		
		boolean aec2 = SmartDashboard.getBoolean("02AuE2", false);
		boolean awb2 = SmartDashboard.getBoolean("02AuWht2", false);
		int gain2 = (int) SmartDashboard.getNumber("02ExpGn2", 50);
		int comp2 = (int) SmartDashboard.getNumber("02ExpComp2", 150);
		int r2 = (int) SmartDashboard.getNumber("02WhtBalRed2", 94);
		int g2 = (int) SmartDashboard.getNumber("02WhteBalGrn2", 94);
		int b2 = (int) SmartDashboard.getNumber("02WhtBalBlu2", 94);
		pixy.setAutoExposure(aec2);
		if (!aec2) {
			pixy2.setExposureCompensation(new ExposureSetting(gain2, comp2));
		}
		pixy2.setAutoWhiteBalance(awb2);
		if (!awb2) {
			pixy2.setWhiteBalanceValue(new WhiteBalanceSetting(r2, g2, b2));
		}
		
	
	}
}

