#include <stdio.h>

#define BIT(nr)			((1UL) << (nr))
#define BIT_ULL(nr)		((1ULL) << (nr))

#define MCG_STATUS_EIPV		BIT_ULL(1)   /* ip points to correct instruction */
#define MCG_STATUS_MCIP		BIT_ULL(2)   /* machine check in progress */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define GDT_ENTRY_KERNEL_CS		2
#define __KERNEL_CS			(GDT_ENTRY_KERNEL_CS*8)

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

struct mce {
	__u64 status;		/* Bank's MCi_STATUS MSR */
	__u64 misc;		/* Bank's MCi_MISC MSR */
	__u64 addr;		/* Bank's MCi_ADDR MSR */
	__u64 mcgstatus;	/* Machine Check Global Status MSR */
	__u64 ip;		/* Instruction Pointer when the error happened */
	__u64 tsc;		/* CPU time stamp counter */
	__u64 time;		/* Wall time_t when error was detected */
	__u8  cpuvendor;	/* Kernel's X86_VENDOR enum */
	__u8  inject_flags;	/* Software inject flags */
	__u8  severity;		/* Error severity */
	__u8  pad;
	__u32 cpuid;		/* CPUID 1 EAX */
	__u8  cs;		/* Code segment */
	__u8  bank;		/* Machine check bank reporting the error */
	__u8  cpu;		/* CPU number; obsoleted by extcpu */
	__u8  finished;		/* Entry is valid */
	__u32 extcpu;		/* Linux CPU number that detected the error */
	__u32 socketid;		/* CPU socket ID */
	__u32 apicid;		/* CPU initial APIC ID */
	__u64 mcgcap;		/* MCGCAP MSR: machine check capabilities of CPU */
	__u64 synd;		/* MCA_SYND MSR: only valid on SMCA systems */
	__u64 ipid;		/* MCA_IPID MSR: only valid on SMCA systems */
	__u64 ppin;		/* Protected Processor Inventory Number */
	__u32 microcode;	/* Microcode revision */
	__u64 kflags;		/* Internal kernel use */
};

static void print_mce(struct mce *m)
{
	printf("CPU %d: Machine Check%s: %Lx Bank %d: %016Lx\n",
		 m->extcpu,
		 (m->mcgstatus & MCG_STATUS_MCIP ? " Exception" : ""),
		 m->mcgstatus, m->bank, m->status);

	if (m->ip) {
		printf("RIP%s %02x:<%016Lx> ",
			!(m->mcgstatus & MCG_STATUS_EIPV) ? " !INEXACT!" : "",
			m->cs, m->ip);

		if (m->cs == __KERNEL_CS)
			printf("{%p}", (void *)(unsigned long)m->ip);
		printf("\n");
	}

	printf("TSC %llx ", m->tsc);
	if (m->addr)
		printf("ADDR %llx ", m->addr);
	if (m->misc)
		printf("MISC %llx ", m->misc);
	if (m->ppin)
		printf("PPIN %llx ", m->ppin);

	if (m->synd || m->ipid /*mce_flags.smca*/) {
		if (m->synd)
			printf("SYND %llx ", m->synd);
		if (m->ipid)
			printf("IPID %llx ", m->ipid);
	}

	printf("\n");

	/*
	 * Note this output is parsed by external tools and old fields
	 * should not be changed.
	 */
	printf("PROCESSOR %u:%x TIME %llu SOCKET %u APIC %x microcode %x\n",
		m->cpuvendor, m->cpuid, m->time, m->socketid, m->apicid,
		m->microcode);
}

struct mce mces_seen[] = {
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0xffffffff938e1c5b,
	  .tsc = 0x71824998ef,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x10,
	  .bank = 0x0,
	  .cpu = 0x0,
	  .finished = 0x0,
	  .extcpu = 0x0,
	  .socketid = 0x0,
	  .apicid = 0x0,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x5628cdd5484b,
	  .tsc = 0x71824978a5,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x1,
	  .finished = 0x0,
	  .extcpu = 0x1,
	  .socketid = 0x0,
	  .apicid = 0x2,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x0,
	  .ip = 0x0,
	  .tsc = 0x0,
	  .time = 0x0,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x0,
	  .cs = 0x0,
	  .bank = 0x0,
	  .cpu = 0x0,
	  .finished = 0x0,
	  .extcpu = 0x0,
	  .socketid = 0x0,
	  .apicid = 0x0,
	  .mcgcap = 0x0,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x0,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0xffffffff93fb47c5,
	  .tsc = 0x718249759f,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x10,
	  .bank = 0x0,
	  .cpu = 0x3,
	  .finished = 0x0,
	  .extcpu = 0x3,
	  .socketid = 0x0,
	  .apicid = 0x6,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4f00c2,
	  .tsc = 0x7182497b05,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x4,
	  .finished = 0x0,
	  .extcpu = 0x4,
	  .socketid = 0x0,
	  .apicid = 0x8,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x7fbc587b49b7,
	  .tsc = 0x7182497909,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x5,
	  .finished = 0x0,
	  .extcpu = 0x5,
	  .socketid = 0x0,
	  .apicid = 0xa,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4e1b8f,
	  .tsc = 0x7182497a97,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x6,
	  .finished = 0x0,
	  .extcpu = 0x6,
	  .socketid = 0x0,
	  .apicid = 0xc,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4e1a76,
	  .tsc = 0x718249782f,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x7,
	  .finished = 0x0,
	  .extcpu = 0x7,
	  .socketid = 0x0,
	  .apicid = 0xe,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0xffffffff93927347,
	  .tsc = 0x71824975fd,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x10,
	  .bank = 0x0,
	  .cpu = 0x8,
	  .finished = 0x0,
	  .extcpu = 0x8,
	  .socketid = 0x0,
	  .apicid = 0x1,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x5628ce047527,
	  .tsc = 0x7182497c95,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0x9,
	  .finished = 0x0,
	  .extcpu = 0x9,
	  .socketid = 0x0,
	  .apicid = 0x3,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x0,
	  .ip = 0x0,
	  .tsc = 0x0,
	  .time = 0x0,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x0,
	  .cs = 0x0,
	  .bank = 0x0,
	  .cpu = 0x0,
	  .finished = 0x0,
	  .extcpu = 0x0,
	  .socketid = 0x0,
	  .apicid = 0x0,
	  .mcgcap = 0x0,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x0,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0xffffffff93927344,
	  .tsc = 0x71824977cf,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x10,
	  .bank = 0x0,
	  .cpu = 0xb,
	  .finished = 0x0,
	  .extcpu = 0xb,
	  .socketid = 0x0,
	  .apicid = 0x7,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4e1b8f,
	  .tsc = 0x7182497a53,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0xc,
	  .finished = 0x0,
	  .extcpu = 0xc,
	  .socketid = 0x0,
	  .apicid = 0x9,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0xffffffff93fb47c5,
	  .tsc = 0x7182497919,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x10,
	  .bank = 0x0,
	  .cpu = 0xd,
	  .finished = 0x0,
	  .extcpu = 0xd,
	  .socketid = 0x0,
	  .apicid = 0xb,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4b8e30,
	  .tsc = 0x7182497adb,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0xe,
	  .finished = 0x0,
	  .extcpu = 0xe,
	  .socketid = 0x0,
	  .apicid = 0xd,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	},
	{
	  .status = 0x0,
	  .misc = 0x0,
	  .addr = 0x0,
	  .mcgstatus = 0x5,
	  .ip = 0x4e19b0,
	  .tsc = 0x7182497827,
	  .time = 0x653a2aca,
	  .cpuvendor = 0x0,
	  .inject_flags = 0x0,
	  .severity = 0x0,
	  .pad = 0x0,
	  .cpuid = 0x606c1,
	  .cs = 0x33,
	  .bank = 0x0,
	  .cpu = 0xf,
	  .finished = 0x0,
	  .extcpu = 0xf,
	  .socketid = 0x0,
	  .apicid = 0xf,
	  .mcgcap = 0xf000c14,
	  .synd = 0x0,
	  .ipid = 0x0,
	  .ppin = 0x0,
	  .microcode = 0x1000230,
	  .kflags = 0x0
	}
};

int main()
{
	int i = 0;

	for(; i < ARRAY_SIZE(mces_seen); i++) {
		printf("[%d]:\n", i);
		print_mce(&mces_seen[i]);
		printf("\n");
	}
	return 0;
}
